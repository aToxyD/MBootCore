#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <functional>
#include <vector>
#include <atomic>

#include <mbootcore/domain/Types.hpp>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>

namespace mbootcore {

class IFlashDevice;
class ILogger;

namespace gpt {
class PartitionManager;
}

namespace session {
class DeviceSession;
}

namespace loader {
class LoaderFramework;
}

namespace pipeline {
class BootPipeline;
}

namespace job {

enum class JobType : uint32_t {
    Flash            = 0,
    Backup           = 1,
    Restore          = 2,
    Read             = 3,
    Erase            = 4,
    Verify           = 5,
    ProgrammerUpload = 6,
    GPTUpdate        = 7,
    Custom           = 8
};

enum class JobState : uint32_t {
    Pending   = 0,
    Preparing = 1,
    Running   = 2,
    Paused    = 3,
    Completed = 4,
    Failed    = 5,
    Cancelled = 6,
    RollingBack = 7,
    RolledBack  = 8
};

enum class JobPriority : uint32_t {
    Low      = 0,
    Normal   = 1,
    High     = 2,
    Critical = 3
};

struct JobStatistics {
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::chrono::milliseconds elapsedTime{0};

    uint64_t processedBytes{0};
    double averageSpeedBps{0.0};
    uint64_t remainingBytes{0};
    std::chrono::milliseconds eta{0};

    uint32_t retryCount{0};
    uint32_t rollbackCount{0};
    uint32_t failureCount{0};
};

struct JobResult {
    bool success{false};
    ErrorCode error{ErrorCode::Success};
    std::string errorMessage;
    JobStatistics statistics;
};

struct JobConfig {
    JobPriority priority{JobPriority::Normal};
    int maxRetries{3};
    int maxRollbacks{1};
    bool enableRollback{true};
    bool enableProgressCallback{true};
    std::chrono::milliseconds timeout{0};
    std::vector<std::string> dependencies;
    std::string description;
    std::string partitionName;
    ByteBuffer data;
    uint64_t address{0};
    uint64_t size{0};
    std::string imagePath;
};

using JobProgressCallback = std::function<void(const std::string& jobId, const ProgressInfo& info)>;

struct JobContext {
    IFlashDevice* flashDevice{nullptr};
    gpt::PartitionManager* partitionManager{nullptr};
    session::DeviceSession* session{nullptr};
    loader::LoaderFramework* loaderFramework{nullptr};
    pipeline::BootPipeline* pipeline{nullptr};
    ILogger* logger{nullptr};
    JobProgressCallback progressCallback;
    std::atomic<bool>* cancelled{nullptr};

    bool isCancelled() const noexcept {
        return cancelled && cancelled->load();
    }
};

} // namespace job
} // namespace mbootcore
