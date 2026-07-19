#pragma once

#include "mbootcore/domain/ITransport.hpp"

#include <deque>
#include <cstdint>
#include <vector>

namespace mbootcore {

class MockTransport : public ITransport {
public:
    struct WriteRecord {
        ByteBuffer data;
        std::chrono::milliseconds timeout;
    };

    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override { return m_open; }

    Result<size_t> write(const ByteBuffer& data,
                         std::chrono::milliseconds timeout) override;
    Result<size_t> read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                        std::chrono::milliseconds timeout) override;
    Result<void> sendZLP(std::chrono::milliseconds timeout) override;
    std::string_view name() const noexcept override { return "MockTransport"; }
    void cancel() noexcept override {}

    transport::TransportType transportType() const noexcept override {
        return transport::TransportType::Mock;
    }
    transport::TransportCapability capabilities() const noexcept override {
        return transport::TransportCapability::Read
             | transport::TransportCapability::Write
             | transport::TransportCapability::Timeout
             | transport::TransportCapability::Cancellation
             | transport::TransportCapability::Configurable
             | transport::TransportCapability::Observable
             | transport::TransportCapability::Reconnectable;
    }
    const transport::TransportConfig& config() const override { return m_config; }
    void setConfig(const transport::TransportConfig& cfg) override { m_config = cfg; }

    // Mock control
    void setReadData(const ByteBuffer& data);
    void setReadResult(Result<size_t> result);
    void setWriteResult(Result<size_t> result);
    void setAutoOpen(bool autoOpen) { m_autoOpen = autoOpen; }

    const std::vector<WriteRecord>& writes() const noexcept { return m_writes; }
    void clearWrites() { m_writes.clear(); }

    void clearReadQueue() { m_readQueue.clear(); }
    void resetReadResult() { m_readResult = Result<size_t>::Error(ErrorCode::TransportError); }

    size_t openCount() const noexcept { return m_openCount; }
    size_t closeCount() const noexcept { return m_closeCount; }

private:
    bool m_open{false};
    bool m_autoOpen{true};
    size_t m_openCount{0};
    size_t m_closeCount{0};
    transport::TransportConfig m_config;
    std::deque<ByteBuffer> m_readQueue;
    Result<size_t> m_readResult{Result<size_t>::Ok(0)};
    Result<size_t> m_writeResult{Result<size_t>::Ok(0)};
    std::vector<WriteRecord> m_writes;
};

inline Result<void> MockTransport::open() {
    m_open = m_autoOpen;
    if (m_autoOpen) ++m_openCount;
    return m_autoOpen ? Result<void>::Ok()
                      : Result<void>::Error(ErrorCode::TransportError);
}

inline void MockTransport::close() noexcept {
    m_open = false;
    ++m_closeCount;
}

inline Result<size_t> MockTransport::write(const ByteBuffer& data,
                                            std::chrono::milliseconds timeout) {
    (void)timeout;
    m_writes.push_back({data, timeout});
    if (m_writeResult.isOk()) {
        return Result<size_t>::Ok(data.size());
    }
    return std::move(m_writeResult);
}

inline Result<size_t> MockTransport::read(ByteBuffer& buffer, size_t minBytes,
                                           size_t maxBytes,
                                            std::chrono::milliseconds timeout) {
    (void)timeout;
    if (!m_readQueue.empty()) {
        auto& next = m_readQueue.front();
        size_t copySize = std::min(next.size(), maxBytes);
        buffer.assign(next.begin(), next.begin() + static_cast<ptrdiff_t>(copySize));
        if (buffer.size() < minBytes) {
            buffer.resize(minBytes);
        }
        m_readQueue.pop_front();
        return Result<size_t>::Ok(buffer.size());
    }
    if (m_readResult.isOk()) {
        buffer.resize(minBytes);
        return Result<size_t>::Ok(minBytes);
    }
    return std::move(m_readResult);
}

inline Result<void> MockTransport::sendZLP(std::chrono::milliseconds timeout) {
    return write({}, timeout).isOk()
        ? Result<void>::Ok()
        : Result<void>::Error(ErrorCode::TransportWriteFailed);
}

inline void MockTransport::setReadData(const ByteBuffer& data) {
    m_readQueue.push_back(data);
    m_readResult = Result<size_t>::Ok(data.size());
}

inline void MockTransport::setReadResult(Result<size_t> result) {
    m_readResult = std::move(result);
}

inline void MockTransport::setWriteResult(Result<size_t> result) {
    m_writeResult = std::move(result);
}

} // namespace mbootcore
