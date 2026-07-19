#include <mbootcore/transport/UsbTransport.hpp>
#include <mbootcore/transport/usb/makeUsbBackend.hpp>
#include <thread>
#include <algorithm>
#include <sstream>

namespace mbootcore {
namespace transport {

struct UsbTransport::Impl {
    std::chrono::steady_clock::time_point lastOpTime;
    bool pendingCancel{false};
};

UsbTransport::UsbTransport(uint16_t vendorId, uint16_t productId,
                           int interfaceNumber, int bulkInEp, int bulkOutEp,
                           ILogger* logger)
    : m_vendorId(vendorId), m_productId(productId)
    , m_interfaceNumber(interfaceNumber), m_bulkInEp(bulkInEp), m_bulkOutEp(bulkOutEp)
    , m_logger(logger)
    , m_impl(std::make_unique<Impl>()) {
    m_backend = usb::makeUsbBackend(logger);
}

UsbTransport::~UsbTransport() = default;

void UsbTransport::setBackend(std::unique_ptr<usb::UsbBackend> backend) {
    m_backend = std::move(backend);
}

Result<void> UsbTransport::open() {
    if (m_state == TransportState::Open) {
        return ErrorCode::TransportAlreadyOpen;
    }
    if (!m_backend) {
        return ErrorCode::TransportBackendUnavailable;
    }

    m_state = TransportState::Opening;
    auto result = m_backend->open(m_vendorId, m_productId, m_interfaceNumber);
    if (result.isError()) {
        m_state = TransportState::Error;
        return result.error();
    }

    m_state = TransportState::Open;
    m_impl->lastOpTime = std::chrono::steady_clock::now();
    return {};
}

void UsbTransport::close() noexcept {
    if (m_backend) m_backend->close();
    m_state = TransportState::Closed;
    m_cancelled = false;
}

bool UsbTransport::isOpen() const noexcept {
    return m_state == TransportState::Open;
}

TransportState UsbTransport::state() const noexcept {
    return m_state;
}

Result<size_t> UsbTransport::write(const ByteBuffer& data, std::chrono::milliseconds timeout) {
    if (m_state != TransportState::Open) {
        return ErrorCode::TransportNotOpen;
    }
    if (m_cancelled) {
        return ErrorCode::TransportAsyncCancelled;
    }

    auto start = std::chrono::steady_clock::now();
    auto result = m_backend->bulkWrite(static_cast<uint8_t>(m_bulkOutEp),
                                       data.data(), data.size(), timeout);
    if (result.isError()) {
        ++m_stats.errorCount;
        return result;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    ++m_stats.writeOperations;
    m_stats.bytesWritten += result.value();
    updateLatency(elapsed);
    updatePeakThroughput(result.value(), elapsed);
    return result;
}

Result<size_t> UsbTransport::read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                                   std::chrono::milliseconds timeout) {
    (void)minBytes;
    if (m_state != TransportState::Open) {
        return ErrorCode::TransportNotOpen;
    }
    if (m_cancelled) {
        return ErrorCode::TransportAsyncCancelled;
    }

    buffer.resize(maxBytes);
    auto start = std::chrono::steady_clock::now();
    auto result = m_backend->bulkRead(static_cast<uint8_t>(m_bulkInEp),
                                      buffer.data(), maxBytes, timeout);
    if (result.isError()) {
        ++m_stats.errorCount;
        return result;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    size_t bytes = result.value();
    buffer.resize(bytes);
    ++m_stats.readOperations;
    m_stats.bytesRead += bytes;
    updateLatency(elapsed);
    updatePeakThroughput(bytes, elapsed);
    return bytes;
}

Result<void> UsbTransport::sendZLP(std::chrono::milliseconds timeout) {
    if (m_state != TransportState::Open) {
        return ErrorCode::TransportNotOpen;
    }
    uint8_t zlp = 0;
    auto result = m_backend->bulkWrite(static_cast<uint8_t>(m_bulkOutEp),
                                       &zlp, 0, timeout);
    if (result.isError()) {
        ++m_stats.errorCount;
        return result.error();
    }
    ++m_stats.writeOperations;
    return {};
}

void UsbTransport::cancel() noexcept {
    m_cancelled = true;
    if (m_backend) {
        (void)m_backend->abortPipe(static_cast<uint8_t>(m_bulkInEp));
        (void)m_backend->abortPipe(static_cast<uint8_t>(m_bulkOutEp));
    }
}

Result<void> UsbTransport::flush() {
    if (m_state != TransportState::Open) {
        return ErrorCode::TransportNotOpen;
    }
    auto result = m_backend->resetPipe(static_cast<uint8_t>(m_bulkInEp));
    if (result.isError()) ++m_stats.errorCount;
    return result;
}

Result<void> UsbTransport::reset() {
    close();
    return open();
}

Result<void> UsbTransport::reconnect() {
    close();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ++m_stats.reconnectCount;
    return open();
}

TransportEndpoint UsbTransport::endpoint() const {
    TransportEndpoint ep;
    ep.type = TransportType::USB;
    ep.vendorId = m_vendorId;
    ep.productId = m_productId;
    return ep;
}

void UsbTransport::updateLatency(std::chrono::microseconds us) {
    double ms = us.count() / 1000.0;
    m_stats.averageLatency = m_stats.averageLatency * 0.7 + ms * 0.3;
}

void UsbTransport::updatePeakThroughput(size_t bytes, std::chrono::microseconds us) {
    if (us.count() == 0) return;
    double bps = (static_cast<double>(bytes) * 1000000.0) / us.count();
    if (bps > m_stats.peakThroughput) m_stats.peakThroughput = bps;
}

} // namespace transport
} // namespace mbootcore
