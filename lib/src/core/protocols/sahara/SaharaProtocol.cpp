#include "mbootcore/core/protocols/sahara/SaharaProtocol.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

namespace mbootcore {

namespace {

uint32_t selectVersion(const HelloPacket& hello) noexcept {
    constexpr uint32_t hostVer = SaharaProtocol::DefaultVersion;
    constexpr uint32_t hostVerSup = SaharaProtocol::DefaultVersionSupported;
    if (hostVer >= hello.versionSupported() && hostVer <= hello.version()) {
        return hostVer;
    }
    if (hello.version() >= hostVerSup && hello.version() <= hostVer) {
        return hello.version();
    }
    return hello.versionSupported();
}

} // namespace

SaharaProtocol::SaharaProtocol(ITransport& transport, ILogger& logger)
    : m_transport(transport)
    , m_logger(logger) {
}

Result<void> SaharaProtocol::handshake() {
    m_logger.info("Sahara", "Starting handshake...");
    m_phase = ProtocolPhase::Handshake;

    auto pktResult = receivePacket(0x01);
    if (!pktResult.isOk()) {
        (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::ErrorOccurred));
        m_phase = ProtocolPhase::Error;
        return pktResult.error();
    }

    auto& hello = static_cast<const HelloPacket&>(*pktResult.value());
    m_logger.info("Sahara", hello.toString());

    m_negotiatedVersion = selectVersion(hello);
    if (m_negotiatedVersion < DefaultVersionSupported ||
        m_negotiatedVersion > DefaultVersion ||
        m_negotiatedVersion < hello.versionSupported() ||
        m_negotiatedVersion > hello.version()) {
        m_errors.push_back(ErrorCode::ProtocolMismatch);
        (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::ErrorOccurred));
        m_phase = ProtocolPhase::Error;
        return ErrorCode::ProtocolMismatch;
    }

    m_deviceInfo.version.major = m_negotiatedVersion;
    m_deviceInfo.mode = saharaModeFromRaw(hello.mode());

    if (m_negotiatedVersion >= 3) {
        m_deviceInfo.capabilities = m_deviceInfo.capabilities | DeviceCapability::V3Supported;
    }

    (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::HelloReceived));

    HelloResponsePacket response(m_negotiatedVersion, DefaultVersionSupported,
                                 HelloResponseSuccess, hello.mode());
    auto sendResult = sendPacket(response);
    if (!sendResult.isOk()) {
        (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::ErrorOccurred));
        m_phase = ProtocolPhase::Error;
        return sendResult.error();
    }

    (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::HelloResponseSent));
    m_logger.info("Sahara", "Negotiated version: " + std::to_string(m_negotiatedVersion));

    // V3: device sends READ_CHIPID_V3 (0x0A) automatically after handshake
    if (m_negotiatedVersion >= 3) {
        auto chipPktResult = receivePacket(0x0A);
        if (!chipPktResult.isOk()) {
            m_logger.warn("Sahara", "V3 chip ID not received, falling back to V2 discovery");
        } else {
            auto& chipId = static_cast<const ReadChipIdPacket&>(*chipPktResult.value());
            m_logger.info("Sahara", chipId.toString());

            m_deviceInfo.chipIdLo = chipId.chipIdLo();
            m_deviceInfo.chipIdHi = chipId.chipIdHi();
            m_deviceInfo.serialNumber = chipId.serialNum();
            m_deviceInfo.id.msmId = chipId.msmId();
            m_deviceInfo.id.oemId = chipId.oemId();
            m_deviceInfo.id.modelId = chipId.modelId();

            m_phase = ProtocolPhase::Idle;
            return {};
        }
    }

    // V2 discovery: query chip info via EXECUTE_REQ/EXECUTE_RESP chain
    Result<void> discResult = performV2Discovery();
    if (!discResult.isOk()) {
        m_logger.warn("Sahara", "V2 discovery incomplete, continuing anyway");
    }

    m_phase = ProtocolPhase::Idle;
    return {};
}

Result<void> SaharaProtocol::uploadProgrammer(const ByteBuffer& programmerData) {
    m_logger.info("Sahara", "Starting programmer upload...");
    m_phase = ProtocolPhase::Transferring;

    while (true) {
        auto pktResult = receivePacket(0x03);
        if (!pktResult.isOk()) {
            return pktResult.error();
        }

        auto& pkt = *pktResult.value();
        if (pkt.command() == 0x04) {
            auto& endImg = static_cast<EndImageTransferPacket&>(pkt);
            if (endImg.status() != 0) {
                m_errors.push_back(ErrorCode::SaharaNakReadDataError);
                return ErrorCode::SaharaNakReadDataError;
            }
            break;
        }

        auto& readPkt = static_cast<const ReadDataPacket&>(pkt);
        (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::ReadDataReceived));

        const size_t offset = static_cast<size_t>(readPkt.dataOffset());
        const size_t length = static_cast<size_t>(readPkt.dataLength());
        const size_t dataSize = programmerData.size();

        if (length > dataSize ||
            offset > dataSize - length) {
            return ErrorCode::InvalidPacket;
        }

        ByteBuffer chunk(programmerData.begin() + static_cast<ptrdiff_t>(offset),
                         programmerData.begin() + static_cast<ptrdiff_t>(offset + length));

        auto writeResult = m_transport.write(chunk, std::chrono::milliseconds(DefaultTimeoutMs));
        if (!writeResult.isOk()) {
            return writeResult.error();
        }

        (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::DataSent));

        if (m_progressCallback) {
            m_progressCallback(offset + length, dataSize);
        }
    }

    (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::EndImageReceived));
    m_phase = ProtocolPhase::WaitingForProgrammer;

    return {};
}

Result<void> SaharaProtocol::reset() {
    m_logger.info("Sahara", "Sending reset command...");

    // Per protocol: DONE_REQ (0x05) -> DONE_RESP (0x06) -> RESET_REQ (0x07) -> RESET_RESP (0x08)
    DonePacket donePkt;
    auto sendResult = sendPacket(donePkt);
    if (!sendResult.isOk()) {
        return sendResult.error();
    }

    auto doneRespResult = receivePacket(0x06);
    if (!doneRespResult.isOk()) {
        return doneRespResult.error();
    }

    ResetPacket resetPkt;
    sendResult = sendPacket(resetPkt);
    if (!sendResult.isOk()) {
        return sendResult.error();
    }

    (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::ResetRequested));

    auto pktResult = receivePacket(0x08);
    if (!pktResult.isOk()) {
        return pktResult.error();
    }

    (void)m_stateMachine.transition(static_cast<uint32_t>(SaharaEvent::ResetResponseRecv));
    m_phase = ProtocolPhase::Finished;

    return {};
}

Result<void> SaharaProtocol::performV2Discovery() {
    struct V2Query {
        uint32_t clientCmd;
        std::string_view name;
    };

    V2Query queries[] = {
        {1, "serial_number"},
        {2, "msm_hw_id"},
        {3, "oem_pk_hash"},
    };

    for (const auto& q : queries) {
        CommandExecPacket execReq(q.clientCmd);
        auto sendResult = sendPacket(execReq);
        if (!sendResult.isOk()) {
            m_logger.warn("Sahara", std::string(q.name) + " req failed");
            return sendResult;
        }

        // Read full packet including response payload
        ByteBuffer rspBuf;
        auto readResult = m_transport.read(rspBuf, 16, 1024,
                                            std::chrono::milliseconds(DefaultTimeoutMs));
        if (!readResult.isOk()) {
            m_logger.warn("Sahara", std::string(q.name) + " response failed");
            return readResult.error();
        }

        auto parseResult = m_parser.parse(rspBuf);
        if (!parseResult.isOk()) {
            return parseResult.error();
        }

        auto& pkt = *parseResult.value();
        if (pkt.command() != 0x0E) {
            return ErrorCode::UnexpectedPacket;
        }

        auto& resp = static_cast<CommandExecResponsePacket&>(pkt);
        if (resp.clientCmd() != q.clientCmd) {
            return ErrorCode::UnexpectedPacket;
        }

        // Extract payload from parsed response (starts after header + cmd + respLength = 16 bytes)
        // The payload follows the CommandExecResponsePacket fields
        size_t payloadOffset = 16;
        auto payloadLen = resp.respLength();
        if (payloadLen > 0 && rspBuf.size() >= payloadOffset + payloadLen) {
            if (q.clientCmd == 1 && payloadLen >= 4) {
                auto* raw = rspBuf.data() + payloadOffset;
                m_deviceInfo.serialNumber =
                    static_cast<uint32_t>(raw[0])
                    | (static_cast<uint32_t>(raw[1]) << 8)
                    | (static_cast<uint32_t>(raw[2]) << 16)
                    | (static_cast<uint32_t>(raw[3]) << 24);
            } else if (q.clientCmd == 2 && payloadLen >= 12) {
                auto* raw = rspBuf.data() + payloadOffset;
                m_deviceInfo.id.msmId =
                    static_cast<uint32_t>(raw[0])
                    | (static_cast<uint32_t>(raw[1]) << 8)
                    | (static_cast<uint32_t>(raw[2]) << 16)
                    | (static_cast<uint32_t>(raw[3]) << 24);
                m_deviceInfo.id.oemId =
                    static_cast<uint32_t>(raw[4])
                    | (static_cast<uint32_t>(raw[5]) << 8)
                    | (static_cast<uint32_t>(raw[6]) << 16)
                    | (static_cast<uint32_t>(raw[7]) << 24);
                m_deviceInfo.id.modelId =
                    static_cast<uint32_t>(raw[8])
                    | (static_cast<uint32_t>(raw[9]) << 8)
                    | (static_cast<uint32_t>(raw[10]) << 16)
                    | (static_cast<uint32_t>(raw[11]) << 24);
            } else if (q.clientCmd == 3 && payloadLen >= 32) {
                auto* raw = rspBuf.data() + payloadOffset;
                for (size_t i = 0; i < 32 && i < m_deviceInfo.id.pkhash.size(); ++i) {
                    m_deviceInfo.id.pkhash[i] = raw[i];
                }
            }
        }

        m_logger.info("Sahara", "V2 discovery: " + std::string(q.name) + " OK");
    }

    return {};
}

void SaharaProtocol::cancel() noexcept {
    m_cancelled = true;
    m_transport.cancel();
}

void SaharaProtocol::resetState() noexcept {
    m_phase = ProtocolPhase::Idle;
    m_errors.clear();
    m_cancelled = false;
    m_stateMachine.reset();
    m_negotiatedVersion = 0;
    m_deviceInfo = DeviceInfo{};
}

Result<std::unique_ptr<IPacket>> SaharaProtocol::receivePacket(uint32_t expectedCmd) {
    ByteBuffer buffer;

    auto readResult = m_transport.read(buffer, 8, 4096,
                                        std::chrono::milliseconds(DefaultTimeoutMs));
    if (!readResult.isOk()) {
        return readResult.error();
    }

    auto parseResult = m_parser.parse(buffer);
    if (!parseResult.isOk()) {
        return parseResult.error();
    }

    auto& packet = *parseResult.value();
    if (packet.command() != expectedCmd && packet.command() != 0x04) {
        return ErrorCode::UnexpectedPacket;
    }

    return parseResult;
}

Result<void> SaharaProtocol::sendPacket(const IPacket& packet) {
    auto serializeResult = m_serializer.serialize(packet);
    if (!serializeResult.isOk()) {
        return serializeResult.error();
    }

    auto writeResult = m_transport.write(serializeResult.value(),
                                          std::chrono::milliseconds(DefaultTimeoutMs));
    if (!writeResult.isOk()) {
        return writeResult.error();
    }

    return {};
}

} // namespace mbootcore
