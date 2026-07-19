#pragma once

#include <mbootcore/transport/network/IUdpBackend.hpp>
#include <mbootcore/domain/Types.hpp>
#include <deque>
#include <vector>
#include <cstring>

namespace mbootcore {
namespace transport {
namespace network {

class MockUdpBackend : public IUdpBackend {
public:
    struct CallRecord {
        enum Type { Open, Close, Send, Recv, Cancel };
        Type type;
        size_t size{0};
    };

    bool isAvailable() const noexcept override { return true; }

    Result<void> open(const std::string& localAddress, uint16_t localPort,
                       const std::string& remoteAddress, uint16_t remotePort,
                       std::chrono::milliseconds timeout,
                       bool broadcast = false) override {
        m_records.push_back({CallRecord::Open, 0});
        m_lastLocalAddr = localAddress;
        m_lastLocalPort = localPort;
        m_lastRemoteAddr = remoteAddress;
        m_lastRemotePort = remotePort;
        m_lastBroadcast = broadcast;
        if (m_failOnOpen) return Result<void>::Error(ErrorCode::TransportError);
        m_isConnected = true;
        return Result<void>::Ok();
    }

    void close() noexcept override {
        m_records.push_back({CallRecord::Close, 0});
        m_isConnected = false;
    }

    bool isConnected() const noexcept override { return m_isConnected; }

    Result<size_t> send(const uint8_t* data, size_t size,
                         std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::Send, size});
        if (m_failOnSend) return Result<size_t>::Error(ErrorCode::TransportWriteFailed);
        m_sends.emplace_back(data, data + size);
        m_totalBytesSent += size;
        return Result<size_t>::Ok(size);
    }

    Result<size_t> recv(uint8_t* buffer, size_t size,
                         std::chrono::milliseconds) override {
        m_records.push_back({CallRecord::Recv, size});
        if (m_failOnRecv) return Result<size_t>::Error(ErrorCode::TransportReadFailed);
        if (!m_recvQueue.empty()) {
            auto& data = m_recvQueue.front();
            size_t copySize = std::min(data.size(), size);
            std::memcpy(buffer, data.data(), copySize);
            m_recvQueue.pop_front();
            m_totalBytesReceived += copySize;
            return Result<size_t>::Ok(copySize);
        }
        if (m_recvTimeout) return Result<size_t>::Error(ErrorCode::TransportTimeout);
        return Result<size_t>::Ok(0);
    }

    void cancel() noexcept override {
        m_records.push_back({CallRecord::Cancel, 0});
        m_cancelled = true;
    }

    std::string backendName() const noexcept override { return "MockUdpBackend"; }
    void setLogger(ILogger*) noexcept override {}

    // Mock control
    void setRecvData(const ByteBuffer& data) { m_recvQueue.push_back(data); }
    void setFailOnOpen(bool v) { m_failOnOpen = v; }
    void setFailOnSend(bool v) { m_failOnSend = v; }
    void setFailOnRecv(bool v) { m_failOnRecv = v; }
    void setRecvTimeout(bool v) { m_recvTimeout = v; }

    const std::vector<CallRecord>& records() const { return m_records; }
    const std::vector<ByteBuffer>& sends() const { return m_sends; }
    size_t totalBytesSent() const { return m_totalBytesSent; }
    size_t totalBytesReceived() const { return m_totalBytesReceived; }
    bool wasCancelled() const { return m_cancelled; }
    void clearRecords() { m_records.clear(); m_sends.clear(); m_cancelled = false; }

    std::string lastLocalAddr() const { return m_lastLocalAddr; }
    uint16_t lastLocalPort() const { return m_lastLocalPort; }
    std::string lastRemoteAddr() const { return m_lastRemoteAddr; }
    uint16_t lastRemotePort() const { return m_lastRemotePort; }
    bool lastBroadcast() const { return m_lastBroadcast; }

private:
    bool m_isConnected{false};
    bool m_cancelled{false};
    bool m_failOnOpen{false};
    bool m_failOnSend{false};
    bool m_failOnRecv{false};
    bool m_recvTimeout{false};
    size_t m_totalBytesSent{0};
    size_t m_totalBytesReceived{0};
    std::string m_lastLocalAddr;
    uint16_t m_lastLocalPort{0};
    std::string m_lastRemoteAddr;
    uint16_t m_lastRemotePort{0};
    bool m_lastBroadcast{false};
    std::deque<ByteBuffer> m_recvQueue;
    std::vector<CallRecord> m_records;
    std::vector<ByteBuffer> m_sends;
};

} // namespace network
} // namespace transport
} // namespace mbootcore
