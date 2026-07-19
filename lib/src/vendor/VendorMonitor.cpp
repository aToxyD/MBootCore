#include <mbootcore/vendor/VendorMonitor.hpp>
#include <algorithm>

namespace mbootcore {
namespace vendor {

void VendorMonitor::recordEvent(const VendorEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    VendorEvent e = event;
    e.timestamp = std::chrono::steady_clock::now();
    m_events.push_back(e);
    if (m_callback) m_callback(e);
}

void VendorMonitor::recordSession(const std::string& vendorId, bool success, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& s = m_stats[vendorId];
    if (success) ++s.successfulSessions;
    else ++s.failedSessions;
    if (s.averageBootTime.count() == 0) s.averageBootTime = duration;
    else s.averageBootTime = std::chrono::milliseconds(
        (s.averageBootTime.count() + duration.count()) / 2);
}

void VendorMonitor::recordBoot(const std::string& vendorId, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& s = m_stats[vendorId];
    if (s.averageBootTime.count() == 0) s.averageBootTime = duration;
    else s.averageBootTime = std::chrono::milliseconds(
        (s.averageBootTime.count() + duration.count()) / 2);
}

void VendorMonitor::recordUpload(const std::string& vendorId) {
    std::lock_guard<std::mutex> lock(m_mutex); ++m_stats[vendorId].uploads;
}

void VendorMonitor::recordFlash(const std::string& vendorId) {
    std::lock_guard<std::mutex> lock(m_mutex); ++m_stats[vendorId].flashes;
}

void VendorMonitor::recordFailure(const std::string& vendorId, ErrorCode error) {
    (void)error;
    std::lock_guard<std::mutex> lock(m_mutex); ++m_stats[vendorId].failedSessions;
}

void VendorMonitor::recordRetry(const std::string& vendorId) {
    (void)vendorId;
}

VendorStatistics VendorMonitor::statistics(const std::string& vendorId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_stats.find(vendorId);
    return it != m_stats.end() ? it->second : VendorStatistics{};
}

std::vector<VendorEvent> VendorMonitor::recentEvents(const std::string& vendorId, size_t maxCount) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<VendorEvent> result;
    for (auto it = m_events.rbegin(); it != m_events.rend() && result.size() < maxCount; ++it) {
        if (it->vendorId == vendorId) result.push_back(*it);
    }
    return result;
}

void VendorMonitor::clear(const std::string& vendorId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.erase(vendorId);
    m_events.erase(std::remove_if(m_events.begin(), m_events.end(),
        [&](const VendorEvent& e) { return e.vendorId == vendorId; }), m_events.end());
}

void VendorMonitor::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.clear(); m_events.clear();
}

size_t VendorMonitor::totalSessions() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t total = 0;
    for (const auto& [id, s] : m_stats) total += s.successfulSessions + s.failedSessions;
    return total;
}

size_t VendorMonitor::totalFailures() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t total = 0;
    for (const auto& [id, s] : m_stats) total += s.failedSessions;
    return total;
}

void VendorMonitor::setEventCallback(EventCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(cb);
}

} // namespace vendor
} // namespace mbootcore
