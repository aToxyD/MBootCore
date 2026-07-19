#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorEvents.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace mbootcore {
namespace vendor {

class VendorMonitor {
public:
    VendorMonitor() = default;
    ~VendorMonitor() = default;

    VendorMonitor(const VendorMonitor&) = delete;
    VendorMonitor& operator=(const VendorMonitor&) = delete;
    VendorMonitor(VendorMonitor&&) = delete;
    VendorMonitor& operator=(VendorMonitor&&) = delete;

    void recordEvent(const VendorEvent& event);
    void recordSession(const std::string& vendorId, bool success, std::chrono::milliseconds duration);
    void recordBoot(const std::string& vendorId, std::chrono::milliseconds duration);
    void recordUpload(const std::string& vendorId);
    void recordFlash(const std::string& vendorId);
    void recordFailure(const std::string& vendorId, ErrorCode error);
    void recordRetry(const std::string& vendorId);

    VendorStatistics statistics(const std::string& vendorId) const;
    std::vector<VendorEvent> recentEvents(const std::string& vendorId, size_t maxCount = 50) const;
    void clear(const std::string& vendorId);
    void clearAll();
    size_t totalSessions() const noexcept;
    size_t totalFailures() const noexcept;

    using EventCallback = std::function<void(const VendorEvent&)>;
    void setEventCallback(EventCallback cb);

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, VendorStatistics> m_stats;
    std::vector<VendorEvent> m_events;
    EventCallback m_callback;
};

} // namespace vendor
} // namespace mbootcore
