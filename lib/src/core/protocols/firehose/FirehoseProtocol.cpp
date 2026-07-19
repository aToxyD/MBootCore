#include "mbootcore/core/protocols/firehose/FirehoseProtocol.hpp"
#include "mbootcore/core/state/StateMachine.hpp"

namespace mbootcore {

FirehoseProtocol::FirehoseProtocol(ITransport& transport, ILogger& logger,
                                   FirehoseConfig config)
    : m_transport(transport)
    , m_logger(logger)
    , m_config(std::move(config)) {
    auto sm = std::make_unique<GenericStateMachine>();
    sm->addState(0, "Idle");
    sm->addState(1, "Configured");
    sm->addState(2, "Busy");
    sm->addState(3, "Error");
    sm->setInitialState(0);
    m_stateMachine = std::move(sm);
}

Result<void> FirehoseProtocol::handshake() {
    m_logger.info("Firehose", "Starting Firehose configure handshake...");
    return performConfigure();
}

Result<void> FirehoseProtocol::performConfigure() {
    ConfigureCommand cmd;
    cmd.memoryName = m_config.memoryName;
    cmd.maxPayloadSizeToTarget = m_config.maxPayloadSizeToTarget;
    cmd.maxPayloadSizeFromTarget = m_config.maxPayloadSizeFromTarget;
    cmd.zlpAwareHost = m_config.zlpAwareHost;
    cmd.mode = m_config.mode;

    for (int attempt = 0; attempt < m_config.maxConfigureRetries; ++attempt) {
        if (m_cancelled) return ErrorCode::Cancelled;

        auto stringResult = FirehoseXmlEngine::serialize(cmd.toXml());
        if (!stringResult.isOk()) {
            m_errors.push_back(stringResult.error());
            continue;
        }

        auto& xml = stringResult.value();
        ByteBuffer buf(xml.begin(), xml.end());
        buf.push_back(0);

        auto writeResult = m_transport.write(buf, m_config.commandTimeout);
        if (!writeResult.isOk()) {
            m_errors.push_back(writeResult.error());
            continue;
        }

        auto responseResult = receiveResponse();
        if (!responseResult.isOk()) {
            m_errors.push_back(responseResult.error());
            continue;
        }

        auto& resp = responseResult.value();
        if (resp.isAck()) {
            m_configured = true;
            m_phase = ProtocolPhase::FirehoseReady;
            (void)m_stateMachine->transition(1);
            m_logger.info("Firehose", "Configure ACK received, Firehose ready.");
            return {};
        }

        if (resp.isNak()) {
            m_logger.warn("Firehose", "Configure NAK: " + resp.nakDescription());
            m_errors.push_back(ErrorCode::ProtocolError);
            continue;
        }
    }

    m_phase = ProtocolPhase::Error;
    return ErrorCode::ProtocolError;
}

Result<void> FirehoseProtocol::uploadProgrammer(const ByteBuffer&) {
    return ErrorCode::NotSupported;
}

Result<void> FirehoseProtocol::reset() {
    m_logger.info("Firehose", "Sending reset...");
    ResetCommand cmd;
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();

    if (result.value().isAck()) {
        m_configured = false;
        m_phase = ProtocolPhase::Finished;
        return {};
    }

    return ErrorCode::ProtocolError;
}

Result<FirehoseResponse> FirehoseProtocol::sendCommand(const FirehoseCommand& cmd) {
    return sendXmlCommand(cmd.serialize());
}

Result<FirehoseResponse> FirehoseProtocol::sendXmlCommand(const std::string& xml) {
    if (m_cancelled) return ErrorCode::Cancelled;

    ByteBuffer buf(xml.begin(), xml.end());
    buf.push_back(0);

    auto writeResult = m_transport.write(buf, m_config.commandTimeout);
    if (!writeResult.isOk()) {
        return writeResult.error();
    }

    return receiveResponse();
}

Result<FirehoseResponse> FirehoseProtocol::receiveResponse() {
    if (m_cancelled) return ErrorCode::Cancelled;

    ByteBuffer buf(8192);
    auto readResult = m_transport.read(buf, 1, 8192, m_config.commandTimeout);
    if (!readResult.isOk()) {
        return readResult.error();
    }

    buf.resize(readResult.value());
    std::string xml(buf.begin(), buf.end());

    auto response = FirehoseResponse::fromXml(xml);
    if (response.isNak()) {
        m_logger.warn("Firehose", response.commandName() + " NAK: " + response.nakDescription());
    }

    return response;
}

Result<FirehoseResponse> FirehoseProtocol::program(const ProgramCommand& cmd,
                                                    const ByteBuffer& data) {
    if (!m_configured) return ErrorCode::InvalidState;
    m_phase = ProtocolPhase::Transferring;

    if (!m_streamEngine) {
        FirehoseStreamEngine::Config streamCfg;
        streamCfg.chunkSizeToTarget = m_config.maxPayloadSizeToTarget;
        streamCfg.chunkSizeFromTarget = m_config.maxPayloadSizeFromTarget;
        streamCfg.chunkTimeout = m_config.chunkTimeout;
        streamCfg.responseTimeout = m_config.commandTimeout;
        m_streamEngine = std::make_unique<FirehoseStreamEngine>(m_transport, streamCfg);
    }

    auto result = m_streamEngine->streamToDevice(cmd, data, m_progressCallback);
    if (!result.isOk()) {
        m_phase = ProtocolPhase::Error;
        return result.error();
    }

    m_phase = ProtocolPhase::FirehoseReady;
    return FirehoseResponse{};
}

Result<ByteBuffer> FirehoseProtocol::read(const ReadCommand& cmd) {
    if (!m_configured) return ErrorCode::InvalidState;
    m_phase = ProtocolPhase::Transferring;

    if (!m_streamEngine) {
        FirehoseStreamEngine::Config streamCfg;
        streamCfg.chunkSizeToTarget = m_config.maxPayloadSizeToTarget;
        streamCfg.chunkSizeFromTarget = m_config.maxPayloadSizeFromTarget;
        streamCfg.chunkTimeout = m_config.chunkTimeout;
        streamCfg.responseTimeout = m_config.commandTimeout;
        m_streamEngine = std::make_unique<FirehoseStreamEngine>(m_transport, streamCfg);
    }

    size_t totalBytes = static_cast<size_t>(cmd.sectorSize) * cmd.numSectorSize;
    auto result = m_streamEngine->streamFromDevice(cmd, totalBytes, m_progressCallback);
    if (!result.isOk()) {
        m_phase = ProtocolPhase::Error;
        return result.error();
    }

    return result;
}

Result<void> FirehoseProtocol::erase(const EraseCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    if (result.value().isNak()) return ErrorCode::ProtocolError;
    return {};
}

Result<ByteBuffer> FirehoseProtocol::peek(const PeekCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    auto& resp = result.value();
    if (resp.isNak()) return ErrorCode::ProtocolError;
    return resp.data();
}

Result<void> FirehoseProtocol::poke(const PokeCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    if (result.value().isNak()) return ErrorCode::ProtocolError;
    return {};
}

Result<void> FirehoseProtocol::patch(const PatchCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    if (result.value().isNak()) return ErrorCode::ProtocolError;
    return {};
}

Result<void> FirehoseProtocol::powerReset(const PowerCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    return {};
}

Result<FirehoseResponse> FirehoseProtocol::getStorageInfo() {
    GetStorageInfoCommand cmd;
    return sendCommand(cmd);
}

Result<ByteBuffer> FirehoseProtocol::getSha256Digest(const GetSha256DigestCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    return result.value().data();
}

Result<void> FirehoseProtocol::configureMemory(const ConfigureMemoryCommand& cmd) {
    auto result = sendCommand(cmd);
    if (!result.isOk()) return result.error();
    if (result.value().isNak()) return ErrorCode::ProtocolError;
    return {};
}

void FirehoseProtocol::cancel() noexcept {
    m_cancelled = true;
    if (m_streamEngine) m_streamEngine->cancel();
}

void FirehoseProtocol::resetState() noexcept {
    m_phase = ProtocolPhase::Idle;
    m_errors.clear();
    m_cancelled = false;
    m_configured = false;
    m_streamEngine.reset();
    if (m_stateMachine) m_stateMachine->reset();
}

} // namespace mbootcore