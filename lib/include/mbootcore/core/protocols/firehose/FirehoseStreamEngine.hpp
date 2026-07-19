#pragma once

#include "mbootcore/domain/Error.hpp"
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/ITransport.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"

#include <functional>
#include <atomic>
#include <chrono>

namespace mbootcore {

class FirehoseStreamEngine {
public:
    struct Config {
        size_t chunkSizeToTarget{1048576};
        size_t chunkSizeFromTarget{1048576};
        std::chrono::milliseconds chunkTimeout{5000};
        std::chrono::milliseconds responseTimeout{5000};
        int maxRetries{1};
    };

    using ProgressCallback = std::function<void(size_t bytesSent, size_t totalBytes)>;

    explicit FirehoseStreamEngine(ITransport& transport, Config config);

    /// Stream data FROM host TO device (for <program>).
    Result<void> streamToDevice(const FirehoseCommand& command,
                                 const ByteBuffer& data,
                                 ProgressCallback progress = nullptr);

    /// Stream data FROM device TO host (for <read>).
    Result<ByteBuffer> streamFromDevice(const FirehoseCommand& command,
                                        size_t totalBytes,
                                        ProgressCallback progress = nullptr);

    void cancel() noexcept { m_cancelled = true; }
    bool isCancelled() const noexcept { return m_cancelled; }
    void reset() noexcept { m_cancelled = false; }

private:
    Result<void> sendCommand(const FirehoseCommand& command);
    Result<FirehoseResponse> receiveResponse();
    Result<void> sendChunk(const uint8_t* data, size_t size);
    Result<ByteBuffer> receiveChunk(size_t size);

    ITransport& m_transport;
    Config m_config;
    std::atomic<bool> m_cancelled{false};
};

} // namespace mbootcore