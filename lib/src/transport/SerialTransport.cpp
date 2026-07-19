#include <mbootcore/transport/SerialTransport.hpp>
#include <mbootcore/transport/serial/makeSerialBackend.hpp>
#include <thread>

namespace mbootcore {
namespace transport {

struct SerialTransport::Impl {
    std::unique_ptr<serial::ISerialBackend> backend;
};

SerialTransport::SerialTransport(const std::string& portName, int baudRate,
                                 int dataBits, int stopBits,
                                 const std::string& parity,
                                 const std::string& flowControl,
                                 ILogger* logger)
    : m_portName(portName), m_baudRate(baudRate), m_dataBits(dataBits), m_stopBits(stopBits)
    , m_parity(parity), m_flowControl(flowControl), m_logger(logger)
    , m_impl(std::make_unique<Impl>()) {
    m_impl->backend = serial::makeSerialBackend(logger);
}

SerialTransport::~SerialTransport() = default;

Result<void> SerialTransport::open() {
    if (m_state == TransportState::Open) {
        return ErrorCode::TransportAlreadyOpen;
    }
    if (!m_impl->backend) {
        return ErrorCode::TransportBackendUnavailable;
    }

    m_state = TransportState::Opening;
    auto result = m_impl->backend->open(m_portName, m_baudRate, m_dataBits, m_stopBits,
                                        m_parity, m_flowControl, m_config.bufferSize);
    if (result.isError()) {
        m_state = TransportState::Error;
        if (m_logger) {
            m_logger->error("SerialTransport", "Failed to open " + m_portName);
        }
        return result;
    }

    m_state = TransportState::Open;
    if (m_logger) {
        m_logger->info("SerialTransport",
            "Opened " + m_portName + " @ " + std::to_string(m_baudRate) + " baud");
    }
    return {};
}

void SerialTransport::close() noexcept {
    if (m_impl->backend) {
        m_impl->backend->close();
    }
    m_state = TransportState::Closed;
    m_cancelled = false;
}

bool SerialTransport::isOpen() const noexcept {
    return m_state == TransportState::Open && m_impl->backend && m_impl->backend->isOpen();
}

TransportState SerialTransport::state() const noexcept {
    if (m_impl->backend && !m_impl->backend->isOpen() && m_state == TransportState::Open) {
        return TransportState::Error;
    }
    return m_state;
}

Result<size_t> SerialTransport::write(const ByteBuffer& data, std::chrono::milliseconds timeout) {
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

Result<size_t> SerialTransport::read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
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

Result<void> SerialTransport::sendZLP(std::chrono::milliseconds timeout) {
    (void)timeout;
    return {};
}

void SerialTransport::cancel() noexcept {
    m_cancelled = true;
    if (m_impl->backend) {
        m_impl->backend->cancel();
    }
}

Result<void> SerialTransport::flush() {
    if (m_state != TransportState::Open) return ErrorCode::TransportNotOpen;
    if (!m_impl->backend) return ErrorCode::TransportBackendUnavailable;
    return m_impl->backend->flush();
}

Result<void> SerialTransport::reset() {
    close();
    return open();
}

Result<void> SerialTransport::reconnect() {
    close();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ++m_stats.reconnectCount;
    return open();
}

TransportEndpoint SerialTransport::endpoint() const {
    TransportEndpoint ep;
    ep.type = TransportType::Serial;
    ep.address = m_portName;
    return ep;
}

} // namespace transport
} // namespace mbootcore
