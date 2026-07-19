#pragma once

#include <mbootcore/transport/network/ITcpBackend.hpp>
#include <mbootcore/domain/Types.hpp>
#include <deque>
#include <vector>
#include <cstring>

namespace mbootcore {
namespace transport {
namespace network {

class MockTcpBackend : public ITcpBackend {
public:
    struct CallRecord {
        enum Type { Open, Close, Write, Read, Cancel, Flush };
        Type type;
        size_t size{0};
    };

    bool isAvailable() const noexcept override { return true; }

    Result<void> open(const std::string& host, uint16_t port,
                       bool keepAlive,
                       std::chrono::milliseconds timeout) override {
        m_records.push_back({CallRecord::Open, 0});
        m_lastHost = host;
        m_lastPort = port;
        if (m_failOnOpen) return Result<void>::Error(ErrorCode::TransportError);
        m_isConnected = true;
        return Result<void>::Ok();
    }

    void close() noexcept override {
        m_records.push_back({CallRecord::Close, 0});
        m_isConnected = false;
    }

    bool isConnected() const noexcept override { return m_isConnected; }

    Result<size_t> write(const uint8_t* data, size_t size,
                          std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::Write, size});
        if (m_failOnWrite) return Result<size_t>::Error(ErrorCode::TransportWriteFailed);
        m_writes.emplace_back(data, data + size);
        m_totalBytesWritten += size;
        return Result<size_t>::Ok(size);
    }

    Result<size_t> read(uint8_t* buffer, size_t size,
                         std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::Read, size});
        if (m_failOnRead) return Result<size_t>::Error(ErrorCode::TransportReadFailed);
        if (!m_readQueue.empty()) {
            auto& data = m_readQueue.front();
            size_t copySize = std::min(data.size(), size);
            std::memcpy(buffer, data.data(), copySize);
            m_readQueue.pop_front();
            m_totalBytesRead += copySize;
            return Result<size_t>::Ok(copySize);
        }
        if (m_readTimeout) return Result<size_t>::Error(ErrorCode::TransportTimeout);
        if (m_disconnectOnEmptyRead) {
            m_isConnected = false;
            return Result<size_t>::Error(ErrorCode::TransportNotOpen);
        }
        return Result<size_t>::Ok(0);
    }

    void cancel() noexcept override {
        m_records.push_back({CallRecord::Cancel, 0});
        m_cancelled = true;
    }

    Result<void> flush() override {
        m_records.push_back({CallRecord::Flush, 0});
        if (m_failOnFlush) return Result<void>::Error(ErrorCode::TransportWriteFailed);
        return Result<void>::Ok();
    }

    std::string backendName() const noexcept override { return "MockTcpBackend"; }
    void setLogger(ILogger*) noexcept override {}

    // Mock control
    void setReadData(const ByteBuffer& data) { m_readQueue.push_back(data); }
    void setFailOnOpen(bool v) { m_failOnOpen = v; }
    void setFailOnWrite(bool v) { m_failOnWrite = v; }
    void setFailOnRead(bool v) { m_failOnRead = v; }
    void setFailOnFlush(bool v) { m_failOnFlush = v; }
    void setReadTimeout(bool v) { m_readTimeout = v; }
    void setDisconnectOnEmptyRead(bool v) { m_disconnectOnEmptyRead = v; }

    const std::vector<CallRecord>& records() const { return m_records; }
    const std::vector<ByteBuffer>& writes() const { return m_writes; }
    size_t totalBytesRead() const { return m_totalBytesRead; }
    size_t totalBytesWritten() const { return m_totalBytesWritten; }
    bool wasCancelled() const { return m_cancelled; }
    void clearRecords() { m_records.clear(); m_writes.clear(); m_cancelled = false; }

    std::string lastHost() const { return m_lastHost; }
    uint16_t lastPort() const { return m_lastPort; }

private:
    bool m_isConnected{false};
    bool m_cancelled{false};
    bool m_failOnOpen{false};
    bool m_failOnWrite{false};
    bool m_failOnRead{false};
    bool m_failOnFlush{false};
    bool m_readTimeout{false};
    bool m_disconnectOnEmptyRead{false};
    size_t m_totalBytesRead{0};
    size_t m_totalBytesWritten{0};
    std::string m_lastHost;
    uint16_t m_lastPort{0};
    std::deque<ByteBuffer> m_readQueue;
    std::vector<CallRecord> m_records;
    std::vector<ByteBuffer> m_writes;
};

} // namespace network
} // namespace transport
} // namespace mbootcore
