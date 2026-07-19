#include <mbootcore/transport/TcpTransport.hpp>
#include <mbootcore/transport/network/makeTcpBackend.hpp>
#include <thread>

namespace mbootcore {
namespace transport {

struct TcpTransport::Impl {
    std::unique_ptr<network::ITcpBackend> backend;
    std::chrono::milliseconds heartbeatInterval{0};
};

TcpTransport::TcpTransport(const std::string& host, uint16_t port,
                           bool keepAlive, ILogger* logger)
    : m_host(host), m_port(port), m_keepAlive(keepAlive), m_logger(logger)
    , m_impl(std::make_unique<Impl>()) {
    m_impl->backend = network::makeTcpBackend(logger);
    m_impl->heartbeatInterval = keepAlive ? std::chrono::seconds(30) : std::chrono::milliseconds(0);
}

TcpTransport::~TcpTransport() = default;

Result<void> TcpTransport::open() {
    if (m_state == TransportState::Open) {
        return ErrorCode::TransportAlreadyOpen;
    }
    if (!m_impl->backend) {
        return ErrorCode::TransportBackendUnavailable;
    }

    m_state = TransportState::Opening;

    auto result = m_impl->backend->open(m_host, m_port, m_keepAlive, m_config.timeout);
    if (result.isError()) {
        m_state = TransportState::Error;
        if (m_logger) m_logger->error("TcpTransport",
            "Connection to " + m_host + ":" + std::to_string(m_port) + " failed");
        return result;
    }

    m_state = TransportState::Open;
    if (m_logger) {
        m_logger->info("TcpTransport",
            "Connected to " + m_host + ":" + std::to_string(m_port));
    }
    return {};
}

void TcpTransport::close() noexcept {
    if (m_impl->backend) {
        m_impl->backend->close();
    }
    m_state = TransportState::Closed;
    m_cancelled = false;
}

bool TcpTransport::isOpen() const noexcept {
    return m_state == TransportState::Open && m_impl->backend && m_impl->backend->isConnected();
}

TransportState TcpTransport::state() const noexcept {
    if (m_impl->backend && !m_impl->backend->isConnected() && m_state == TransportState::Open) {
        return TransportState::Error;
    }
    return m_state;
}

Result<size_t> TcpTransport::write(const ByteBuffer& data, std::chrono::milliseconds timeout) {
    if (m_state != TransportState::Open) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

    auto start = std::chrono::steady_clock::now();
    auto result = m_impl->backend->write(data.data(), data.size(), timeout);

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

Result<size_t> TcpTransport::read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                                   std::chrono::milliseconds timeout) {
    if (m_state != TransportState::Open) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::TransportAsyncCancelled;
    if (!m_impl->backend) return ErrorCode::TransportBackendUnavailable;

    auto start = std::chrono::steady_clock::now();

    // Use resize(0) + resize(maxBytes) instead of clear() + reserve()
    // to ensure data() returns a valid writable range.
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
        auto result = m_impl->backend->read(
            buffer.data() + totalRead, toRead, subTimeout);

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

Result<void> TcpTransport::sendZLP(std::chrono::milliseconds timeout) {
    (void)timeout;
    return {};
}

void TcpTransport::cancel() noexcept {
    m_cancelled = true;
    if (m_impl->backend) {
        m_impl->backend->cancel();
    }
}

Result<void> TcpTransport::flush() {
    if (m_state != TransportState::Open) return ErrorCode::TransportNotOpen;
    if (!m_impl->backend) return ErrorCode::TransportBackendUnavailable;
    return m_impl->backend->flush();
}

Result<void> TcpTransport::reset() {
    close();
    return open();
}

Result<void> TcpTransport::reconnect() {
    close();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ++m_stats.reconnectCount;
    return open();
}

TransportEndpoint TcpTransport::endpoint() const {
    TransportEndpoint ep;
    ep.type = TransportType::TCP;
    ep.address = m_host;
    ep.port = m_port;
    return ep;
}

} // namespace transport
} // namespace mbootcore
