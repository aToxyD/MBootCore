#pragma once

#include "mbootcore/domain/Error.hpp"
#include "mbootcore/domain/Types.hpp"

#include <functional>
#include <chrono>
#include <atomic>
#include <cstdint>

namespace mbootcore {

class ChunkEngine {
public:
    struct Config {
        size_t chunkSize{1048576};
        std::chrono::milliseconds chunkTimeout{10000};
        std::chrono::milliseconds responseTimeout{5000};
        int maxRetries{1};
        size_t bufferPoolSize{0};
    };

    using ChunkWriteFn = std::function<Result<void>(const uint8_t* data, size_t size)>;
    using ChunkReadFn = std::function<Result<ByteBuffer>(size_t size)>;
    using ResponseFn = std::function<Result<void>()>;
    using ProgressFn = std::function<void(size_t transferred, size_t total)>;

    explicit ChunkEngine(Config config);

    Result<void> streamToTarget(const uint8_t* data, size_t totalSize,
                                ChunkWriteFn writeFn, ResponseFn responseFn,
                                ProgressFn progress = nullptr);

    Result<ByteBuffer> streamFromTarget(size_t totalSize,
                                         ChunkReadFn readFn, ResponseFn responseFn,
                                         ProgressFn progress = nullptr);

    Result<void> streamToTargetPartial(const uint8_t* data, size_t totalSize,
                                        size_t offset, size_t length,
                                        ChunkWriteFn writeFn, ResponseFn responseFn,
                                        ProgressFn progress = nullptr);

    void cancel() noexcept;
    bool isCancelled() const noexcept;
    void reset() noexcept;

    size_t chunkSize() const noexcept { return m_config.chunkSize; }

    void setResumeOffset(size_t offset) noexcept { m_resumeOffset = offset; }
    size_t resumeOffset() const noexcept { return m_resumeOffset; }

private:
    Config m_config;
    std::atomic<bool> m_cancelled{false};
    struct BufferPool {
        std::vector<ByteBuffer> buffers;
        size_t poolSize;
    };
    BufferPool m_bufferPool;
    size_t m_resumeOffset{0};
};

} // namespace mbootcore
