#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>

#include <functional>
#include <string>
#include <cstdint>

namespace mbootcore {
namespace runtime {

struct RuntimeCallbacks {
    std::function<void(const std::string& message)> onLog;
    std::function<void(const std::string& message)> onError;
    std::function<void(const std::string& message)> onWarning;
    std::function<void(const std::string& message)> onStatus;

    std::function<void(const discovery::DeviceDescriptor& device)> onDeviceDiscovered;
    std::function<void(const discovery::DeviceDescriptor& device)> onDeviceConnected;

    std::function<void(const std::string& op, double progress)> onProgress;
    std::function<void(const workflow::WorkflowProgress& progress)> onWorkflowProgress;
    std::function<void(const std::string& jobId, const ProgressInfo& progress)> onJobProgress;
    std::function<void(const std::string& step, float progress)> onPackageProgress;
};

} // namespace runtime
} // namespace mbootcore
