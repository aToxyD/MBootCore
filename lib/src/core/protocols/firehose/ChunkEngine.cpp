#include "mbootcore/core/protocols/firehose/ChunkEngine.hpp"

#include <algorithm>
#include <cstring>

namespace mbootcore {

ChunkEngine::ChunkEngine(Config config)
    : m_config(std::move(config)) {
    if (m_config.bufferPoolSize > 0) {
        m_bufferPool.poolSize = m_config.bufferPoolSize;
        m_bufferPool.buffers.reserve(m_config.bufferPoolSize);
        for (size_t i = 0; i < m_config.bufferPoolSize; ++i) {
            m_bufferPool.buffers.emplace_back(m_config.chunkSize);
        }
    }
}

Result<void> ChunkEngine::streamToTarget(const uint8_t* data, size_t totalSize,
                                          ChunkWriteFn writeFn, ResponseFn responseFn,
                                          ProgressFn progress) {
    if (m_cancelled) return ErrorCode::Cancelled;

    size_t totalSent = 0;
    size_t remaining = totalSize;

    while (remaining > 0 && !m_cancelled) {
        size_t chunkSize = std::min(remaining, m_config.chunkSize);
        const uint8_t* chunkStart = data + totalSent;

        Result<void> writeResult = writeFn(chunkStart, chunkSize);
        if (!writeResult.isOk()) {
            if (m_config.maxRetries > 0) {
                writeResult = writeFn(chunkStart, chunkSize);
            }
            if (!writeResult.isOk()) {
                return writeResult;
            }
        }

        if (responseFn) {
            auto respResult = responseFn();
            if (!respResult.isOk()) {
                return respResult;
            }
        }

        totalSent += chunkSize;
        remaining -= chunkSize;

        if (progress) {
            progress(totalSent, totalSize);
        }
    }

    if (m_cancelled) return ErrorCode::Cancelled;

    m_resumeOffset = totalSent;
    return {};
}

Result<ByteBuffer> ChunkEngine::streamFromTarget(size_t totalSize,
                                                   ChunkReadFn readFn,
                                                   ResponseFn responseFn,
                                                   ProgressFn progress) {
    if (m_cancelled) return ErrorCode::Cancelled;

    if (responseFn) {
        auto respResult = responseFn();
        if (!respResult.isOk()) {
            return respResult.error();
        }
    }

    ByteBuffer allData;
    allData.reserve(totalSize);
    size_t totalReceived = 0;
    size_t remaining = totalSize;

    while (remaining > 0 && !m_cancelled) {
        size_t chunkSize = std::min(remaining, m_config.chunkSize);
        auto chunkResult = readFn(chunkSize);
        if (!chunkResult.isOk()) {
            return chunkResult.error();
        }
        auto& chunk = chunkResult.value();
        allData.insert(allData.end(), chunk.begin(), chunk.end());
        totalReceived += chunk.size();
        remaining -= chunk.size();
        if (progress) {
            progress(totalReceived, totalSize);
        }
    }

    if (m_cancelled) return ErrorCode::Cancelled;

    return allData;
}

Result<void> ChunkEngine::streamToTargetPartial(const uint8_t* data, size_t totalSize,
                                                  size_t offset, size_t length,
                                                  ChunkWriteFn writeFn, ResponseFn responseFn,
                                                  ProgressFn progress) {
    if (offset >= totalSize) {
        return ErrorCode::InvalidArgument;
    }
    size_t clampedLength = std::min(length, totalSize - offset);
    return streamToTarget(data + offset, clampedLength, std::move(writeFn),
                          std::move(responseFn), std::move(progress));
}

void ChunkEngine::cancel() noexcept {
    m_cancelled = true;
}

bool ChunkEngine::isCancelled() const noexcept {
    return m_cancelled;
}

void ChunkEngine::reset() noexcept {
    m_cancelled = false;
    m_resumeOffset = 0;
}

} // namespace mbootcore
