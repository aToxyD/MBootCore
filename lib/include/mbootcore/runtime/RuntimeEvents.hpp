#pragma once

#include <mbootcore/domain/Error.hpp>

#include <cstdint>
#include <string>
#include <chrono>

namespace mbootcore {
namespace runtime {

enum class RuntimeEventType : uint32_t {
    RuntimeStarted,
    RuntimeStopped,
    DeviceConnected,
    DeviceDisconnected,
    WorkflowStarted,
    WorkflowFinished,
    WorkflowFailed,
    JobStarted,
    JobFinished,
    PackageLoaded,
    PluginLoaded,
    VendorLoaded,
    TransportLost,
    TransportRecovered
};

struct RuntimeEvent {
    RuntimeEventType type{RuntimeEventType::RuntimeStarted};
    std::string source;
    std::string message;
    bool success{true};
    ErrorCode error{ErrorCode::Success};
    std::chrono::steady_clock::time_point timestamp;
};

} // namespace runtime
} // namespace mbootcore
