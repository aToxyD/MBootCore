#include <mbootcore/transport/UdpTransport.hpp>
#include <mbootcore/transport/network/makeUdpBackend.hpp>
#include <thread>

namespace mbootcore {
namespace transport {

struct UdpTransport::Impl {
    std::unique_ptr<network::IUdpBackend> backend;
};

UdpTransport::UdpTransport(const std::string& localAddr, uint16_t localPort,
                           const std::string& remoteAddr, uint16_t remotePort,
                           bool broadcast, ILogger* logger)
    : m_localAddr(localAddr), m_remoteAddr(remoteAddr)
    , m_localPort(localPort), m_remotePort(remotePort)
    , m_broadcast(broadcast), m_logger(logger)
    , m_impl(std::make_unique<Impl>()) {
    m_impl->backend = network::makeUdpBackend(logger);
}

UdpTransport::~UdpTransport() = default;

Result<void> UdpTransport::open() {
    if (m_state == TransportState::Open) {
        return ErrorCode::TransportAlreadyOpen;
    }
    if (!m_impl->backend) {
        return ErrorCode::TransportBackendUnavailable;
    }

    m_state = TransportState::Opening;

    auto result = m_impl->backend->open(
        m_localAddr, m_localPort, m_remoteAddr, m_remotePort,
        m_config.timeout, m_broadcast);
    if (result.isError()) {
        m_state = TransportState::Error;
        if (m_logger) m_logger->error("UdpTransport",
            "UDP open failed " + m_localAddr + ":" + std::to_string(m_localPort) +
            " -> " + m_remoteAddr + ":" + std::to_string(m_remotePort));
        return result;
    }

    m_state = TransportState::Open;
    if (m_logger) {
        m_logger->info("UdpTransport",
            "UDP open " + m_localAddr + ":" + std::to_string(m_localPort) +
            " -> " + m_remoteAddr + ":" + std::to_string(m_remotePort));
    }
    return {};
}

void UdpTransport::close() noexcept {
    if (m_impl->backend) {
        m_impl->backend->close();
    }
    m_state = TransportState::Closed;
    m_cancelled = false;
}

bool UdpTransport::isOpen() const noexcept {
    return m_state == TransportState::Open && m_impl->backend && m_impl->backend->isConnected();
}

TransportState UdpTransport::state() const noexcept {
    if (m_impl->backend && !m_impl->backend->isConnected() && m_state == TransportState::Open) {
        return TransportState::Error;
    }
    return m_state;
}

Result<size_t> UdpTransport::write(const ByteBuffer& data, std::chrono::milliseconds timeout) {
    if (m_state != TransportState::Open) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

    auto start = std::chrono::steady_clock::now();
    auto result = m_impl->backend->send(data.data(), data.size(), timeout);

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);

    if (result.isError()) {
        ++m_stats.errorCount;
        if (result.error() == ErrorCode::TransportTimeout) {
            ++m_stats.timeoutCount;
        }
        return result;
    }

    ++m_stats.writeOperations;
    m_stats.bytesWritten += static_cast<uint64_t>(result.value());
    m_stats.averageLatency = m_stats.averageLatency * 0.7 +
        (elapsed.count() / 1000.0) * 0.3;
    return result;
}

Result<size_t> UdpTransport::read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                                   std::chrono::milliseconds timeout) {
    if (m_state != TransportState::Open) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::TransportAsyncCancelled;
    if (!m_impl->backend) return ErrorCode::TransportBackendUnavailable;

    auto start = std::chrono::steady_clock::now();

    buffer.resize(0);
    buffer.resize(maxBytes);

    auto deadline = start + timeout;
    size_t totalRead = 0;

    while (totalRead < minBytes) {
        if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            ++m_stats.timeoutCount;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        auto subTimeout = std::min(remaining, std::chrono::milliseconds(100));

        size_t toRead = maxBytes - totalRead;
        auto result = m_impl->backend->recv(
            reinterpret_cast<uint8_t*>(buffer.data()) + totalRead, toRead, subTimeout);

        if (result.isError()) {
            if (result.error() == ErrorCode::TransportTimeout) continue;
            ++m_stats.errorCount;
            return result;
        }

        size_t bytesRead = result.value();
        if (bytesRead > 0) {
            totalRead += bytesRead;
        }
    }

    buffer.resize(totalRead);

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    ++m_stats.readOperations;
    m_stats.bytesRead += buffer.size();
    m_stats.averageLatency = m_stats.averageLatency * 0.7 +
        (elapsed.count() / 1000.0) * 0.3;

    return buffer.size();
}

Result<void> UdpTransport::sendZLP(std::chrono::milliseconds timeout) {
    (void)timeout;
    return {};
}

void UdpTransport::cancel() noexcept {
    m_cancelled = true;
    if (m_impl->backend) {
        m_impl->backend->cancel();
    }
}

Result<void> UdpTransport::flush() {
    return {};
}

Result<void> UdpTransport::reset() {
    close();
    return open();
}

Result<void> UdpTransport::reconnect() {
    close();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ++m_stats.reconnectCount;
    return open();
}

TransportEndpoint UdpTransport::endpoint() const {
    TransportEndpoint ep;
    ep.type = TransportType::UDP;
    ep.address = m_remoteAddr;
    ep.port = m_remotePort;
    return ep;
}

} // namespace transport
} // namespace mbootcore
