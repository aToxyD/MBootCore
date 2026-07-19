#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <string>
#include <functional>

namespace mbootcore {
namespace vendor {

enum class VendorEventType : uint32_t {
    VendorLoaded = 0,
    VendorUnloaded,
    SessionOpened,
    SessionClosed,
    CapabilityChanged,
    PluginLoaded,
    PluginFailed,
    DiscoveryCompleted,
    WorkflowStarted,
    WorkflowFinished
};

struct VendorEvent {
    VendorEventType type{VendorEventType::VendorLoaded};
    std::string vendorId;
    std::string message;
    bool success{true};
    std::chrono::steady_clock::time_point timestamp;
};

using VendorEventCallback = std::function<void(const VendorEvent&)>;

} // namespace vendor
} // namespace mbootcore
