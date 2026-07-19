#pragma once

#include <mbootcore/domain/Types.hpp>
#include <mbootcore/domain/Error.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <unordered_map>

namespace mbootcore {

namespace session { class DeviceSession; }
namespace pipeline { class BootPipeline; }
namespace job { class JobPipeline; }
namespace job { struct JobContext; }
namespace firmware {
    class FirmwarePackage;
    class FlashPlanGenerator;
    class FirmwareExecutor;
}
class LoaderFramework;
class IFlashDevice;
namespace plugin { class PluginManager; }

namespace workflow {

enum class WorkflowState : uint32_t {
    Created   = 0,
    Ready     = 1,
    Running   = 2,
    Paused    = 3,
    Cancelled = 4,
    Completed = 5,
    Failed    = 6,
    RollingBack = 7
};

enum class WorkflowStepType : uint32_t {
    Connect       = 0,
    Detect        = 1,
    Negotiate     = 2,
    UploadLoader  = 3,
    Flash         = 4,
    Verify        = 5,
    GPT           = 6,
    Backup        = 7,
    Restore       = 8,
    Reboot        = 9,
    Disconnect    = 10,
    Custom        = 99
};

enum class WorkflowPriority : uint32_t {
    Low      = 0,
    Normal   = 1,
    High     = 2,
    Critical = 3
};

struct WorkflowStatistics {
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point finishTime;
    std::chrono::milliseconds elapsed{0};
    uint32_t retries{0};
    uint32_t rollbacks{0};
    uint32_t warnings{0};
    uint32_t errors{0};
};

struct WorkflowProgress {
    double overallProgress{0.0};
    std::string currentStep;
    double currentStepProgress{0.0};
    std::chrono::seconds eta{0};
    uint64_t bytesTransferred{0};
};

struct WorkflowResult {
    bool success{false};
    ErrorCode error{ErrorCode::Success};
    std::string message;
    WorkflowStatistics statistics;
};

struct WorkflowOptions {
    bool autoRollback{true};
    bool autoReconnect{false};
    int retryCount{3};
    std::chrono::milliseconds timeout{0};
    bool continueOnWarning{false};
};

struct WorkflowContext {
    session::DeviceSession* session{nullptr};
    pipeline::BootPipeline* bootPipeline{nullptr};
    job::JobPipeline* jobPipeline{nullptr};
    job::JobContext* jobContext{nullptr};
    firmware::FirmwarePackage* firmwarePackage{nullptr};
    firmware::FlashPlanGenerator* flashPlanGenerator{nullptr};
    firmware::FirmwareExecutor* firmwareExecutor{nullptr};
    LoaderFramework* loaderFramework{nullptr};
    IFlashDevice* flashDevice{nullptr};
    plugin::PluginManager* pluginManager{nullptr};

    std::unordered_map<std::string, std::string> properties;

    bool isCancelled() const noexcept {
        auto it = properties.find("cancelled");
        return it != properties.end() && it->second == "true";
    }
};

} // namespace workflow
} // namespace mbootcore
