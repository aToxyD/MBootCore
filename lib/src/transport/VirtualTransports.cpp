#include <mbootcore/transport/VirtualTransports.hpp>
#include <algorithm>
#include <cstring>
#include <random>

namespace mbootcore {
namespace transport {

VirtualUsbTransport::VirtualUsbTransport() : m_rng(std::random_device{}()) {
    m_startTime = std::chrono::steady_clock::now();
}
VirtualUsbTransport::VirtualUsbTransport(const VirtualTransportConfig& config)
    : m_virtConfig(config), m_rng(std::random_device{}()) {
    m_startTime = std::chrono::steady_clock::now();
}
VirtualUsbTransport::~VirtualUsbTransport() = default;

Result<void> VirtualUsbTransport::open() {
    if (m_virtConfig.failOnOpen) { m_state = TransportState::Error; return ErrorCode::TransportError; }
    m_state = TransportState::Open; m_simDisconnected = false; return {};
}
void VirtualUsbTransport::close() noexcept { m_state = TransportState::Closed; m_cancelled = false; m_simDisconnected = false; }
bool VirtualUsbTransport::isOpen() const noexcept { return m_state == TransportState::Open && !m_simDisconnected; }
TransportState VirtualUsbTransport::state() const noexcept {
    if (m_simDisconnected) return TransportState::Error;
    return m_state;
}
void VirtualUsbTransport::injectData(const ByteBuffer& data) { m_readQueue.push_back(data); m_injected += data.size(); }
void VirtualUsbTransport::triggerDisconnect() { m_simDisconnected = true; m_state = TransportState::Error; }
void VirtualUsbTransport::triggerReconnect() { m_simDisconnected = false; m_state = TransportState::Open; }

void VirtualUsbTransport::applyLatency() {
    if (!m_virtConfig.simulateLatency) return;
    std::uniform_int_distribution<int> d(static_cast<int>(m_virtConfig.minLatency.count()), static_cast<int>(m_virtConfig.maxLatency.count()));
    std::this_thread::sleep_for(std::chrono::milliseconds(d(m_rng)));
}

bool VirtualUsbTransport::shouldDrop() {
    if (!m_virtConfig.simulatePacketLoss) return false;
    return std::uniform_real_distribution<double>(0,1)(m_rng) < m_virtConfig.packetLossRate;
}
bool VirtualUsbTransport::shouldCorrupt() {
    if (!m_virtConfig.simulateCorruption) return false;
    return std::uniform_real_distribution<double>(0,1)(m_rng) < m_virtConfig.corruptionRate;
}
void VirtualUsbTransport::maybeCorrupt(ByteBuffer& data) {
    if (!shouldCorrupt() || data.empty()) return;
    data[std::uniform_int_distribution<size_t>(0, data.size()-1)(m_rng)] = std::uniform_int_distribution<uint8_t>(0,255)(m_rng);
}
bool VirtualUsbTransport::shouldDuplicate() {
    if (!m_virtConfig.simulateDuplication) return false;
    return std::uniform_real_distribution<double>(0,1)(m_rng) < m_virtConfig.duplicationRate;
}
bool VirtualUsbTransport::shouldTruncate() {
    if (!m_virtConfig.simulateTruncation) return false;
    return std::uniform_real_distribution<double>(0,1)(m_rng) < m_virtConfig.truncationRate;
}

Result<size_t> VirtualUsbTransport::write(const ByteBuffer& data, std::chrono::milliseconds timeout) {
    (void)timeout;
    if (!isOpen()) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::Cancelled;
    if (m_virtConfig.failOnWrite) return ErrorCode::TransportWriteFailed;
    applyLatency();
    if (shouldDrop()) { ++m_stats.errorCount; return data.size(); }
    if (m_faultConfig.enableWriteTimeout) return ErrorCode::TransportTimeout;
    ++m_stats.writeOperations;
    m_stats.bytesWritten += data.size();
    m_bytesWritten += data.size();
    if (shouldDuplicate()) m_readQueue.push_back(data);
    return data.size();
}

Result<size_t> VirtualUsbTransport::read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes, std::chrono::milliseconds timeout) {
    (void)timeout;
    if (!isOpen()) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::Cancelled;
    if (m_virtConfig.failOnRead) return ErrorCode::TransportReadFailed;
    applyLatency();
    if (m_faultConfig.enableReadTimeout) return ErrorCode::TransportTimeout;
    if (m_readQueue.empty()) { ++m_stats.timeoutCount; return ErrorCode::TransportTimeout; }
    ByteBuffer data = std::move(m_readQueue.front());
    m_readQueue.pop_front();
    if (shouldTruncate() && !data.empty()) data.resize(data.size() / 2);
    maybeCorrupt(data);
    size_t copySize = std::min(data.size(), maxBytes);
    buffer.assign(data.begin(), data.begin() + static_cast<ptrdiff_t>(copySize));
    if (buffer.size() < minBytes && !buffer.empty()) buffer.resize(minBytes);
    ++m_stats.readOperations;
    m_stats.bytesRead += buffer.size();
    m_bytesRead += buffer.size();
    return buffer.size();
}

Result<void> VirtualUsbTransport::sendZLP(std::chrono::milliseconds timeout) {
    (void)timeout;
    if (!isOpen()) return ErrorCode::TransportNotOpen;
    ++m_stats.writeOperations;
    return {};
}
void VirtualUsbTransport::cancel() noexcept { m_cancelled = true; }
Result<void> VirtualUsbTransport::flush() { m_readQueue.clear(); return {}; }
Result<void> VirtualUsbTransport::reset() { close(); m_readQueue.clear(); return open(); }
Result<void> VirtualUsbTransport::reconnect() { close(); m_readQueue.clear(); return open(); }

TransportEndpoint VirtualUsbTransport::endpoint() const {
    TransportEndpoint ep; ep.type = TransportType::Virtual; ep.description = "Virtual USB Transport"; return ep;
}

} // namespace transport
} // namespace mbootcore
