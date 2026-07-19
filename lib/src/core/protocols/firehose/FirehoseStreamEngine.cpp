#include "mbootcore/core/protocols/firehose/FirehoseStreamEngine.hpp"

#include <cstring>

namespace mbootcore {

FirehoseStreamEngine::FirehoseStreamEngine(ITransport& transport, Config config)
    : m_transport(transport)
    , m_config(std::move(config)) {
}

Result<void> FirehoseStreamEngine::sendCommand(const FirehoseCommand& command) {
    if (m_cancelled) return ErrorCode::Cancelled;

    auto cmdResult = FirehoseXmlEngine::serialize(command.toXml());
    if (!cmdResult.isOk()) {
        return cmdResult.error();
    }

    auto& xml = cmdResult.value();
    ByteBuffer buf(xml.begin(), xml.end());
    buf.push_back(0); // null terminator

    auto writeResult = m_transport.write(buf, m_config.responseTimeout);
    if (!writeResult.isOk()) {
        return writeResult.error();
    }
    return {};
}

Result<FirehoseResponse> FirehoseStreamEngine::receiveResponse() {
    if (m_cancelled) return ErrorCode::Cancelled;

    // Read up to 8KB response
    ByteBuffer buf(8192);
    auto readResult = m_transport.read(buf, 1, 8192, m_config.responseTimeout);
    if (!readResult.isOk()) {
        return readResult.error();
    }

    size_t bytesRead = readResult.value();
    buf.resize(bytesRead);

    std::string xml(buf.begin(), buf.end());
    return FirehoseResponse::fromXml(xml);
}

Result<void> FirehoseStreamEngine::sendChunk(const uint8_t* data, size_t size) {
    if (m_cancelled) return ErrorCode::Cancelled;

    ByteBuffer buf(data, data + size);
    auto result = m_transport.write(buf, m_config.chunkTimeout);
    if (!result.isOk()) {
        return result.error();
    }
    return {};
}

Result<ByteBuffer> FirehoseStreamEngine::receiveChunk(size_t size) {
    if (m_cancelled) return ErrorCode::Cancelled;

    ByteBuffer buf(size);
    auto result = m_transport.read(buf, size, size, m_config.chunkTimeout);
    if (!result.isOk()) {
        return result.error();
    }
    buf.resize(result.value());
    return buf;
}

Result<void> FirehoseStreamEngine::streamToDevice(const FirehoseCommand& command,
                                                    const ByteBuffer& data,
                                                    ProgressCallback progress) {
    if (m_cancelled) return ErrorCode::Cancelled;

    // Send the command XML
    auto cmdResult = sendCommand(command);
    if (!cmdResult.isOk()) return cmdResult;

    // Stream data in chunks
    size_t totalSent = 0;
    size_t remaining = data.size();

    while (remaining > 0 && !m_cancelled) {
        size_t chunkSize = std::min(remaining, m_config.chunkSizeToTarget);
        const uint8_t* chunkStart = data.data() + totalSent;

        // Send chunk
        auto writeResult = sendChunk(chunkStart, chunkSize);
        if (!writeResult.isOk()) {
            if (m_config.maxRetries > 0) {
                writeResult = sendChunk(chunkStart, chunkSize);
            }
            if (!writeResult.isOk()) {
                return writeResult.error();
            }
        }

        // Receive ACK
        auto respResult = receiveResponse();
        if (!respResult.isOk()) {
            return respResult.error();
        }

        auto& resp = respResult.value();
        if (resp.isNak()) {
            return ErrorCode::ProtocolError;
        }

        totalSent += chunkSize;
        remaining -= chunkSize;

        if (progress) {
            progress(totalSent, data.size());
        }
    }

    if (m_cancelled) return ErrorCode::Cancelled;

    return {};
}

Result<ByteBuffer> FirehoseStreamEngine::streamFromDevice(const FirehoseCommand& command,
                                                           size_t totalBytes,
                                                           ProgressCallback progress) {
    if (m_cancelled) return ErrorCode::Cancelled;

    // Send the command XML
    auto cmdResult = sendCommand(command);
    if (!cmdResult.isOk()) {
        return cmdResult.error();
    }

    // Receive response (should be ACK with raw_mode)
    auto respResult = receiveResponse();
    if (!respResult.isOk()) {
        return respResult.error();
    }

    auto& resp = respResult.value();
    if (resp.isNak()) {
        return ErrorCode::ProtocolError;
    }

    ByteBuffer allData;
    allData.reserve(totalBytes);
    size_t totalReceived = 0;
    size_t remaining = totalBytes;

    while (remaining > 0 && !m_cancelled) {
        size_t chunkSize = std::min(remaining, m_config.chunkSizeFromTarget);

        auto chunkResult = receiveChunk(chunkSize);
        if (!chunkResult.isOk()) {
            return chunkResult.error();
        }

        auto& chunk = chunkResult.value();
        allData.insert(allData.end(), chunk.begin(), chunk.end());
        totalReceived += chunk.size();
        remaining -= chunk.size();

        if (progress) {
            progress(totalReceived, totalBytes);
        }
    }

    if (m_cancelled) return ErrorCode::Cancelled;

    return allData;
}

} // namespace mbootcore