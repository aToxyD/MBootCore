#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace mbootcore { namespace stress {

enum class VirtualDeviceType : uint32_t { Generic=0, Sahara=1, Firehose=2, Composite=3 };
enum class FailureMode : uint32_t { None=0, RandomDisconnect=1, Timeout=2, TransportCorruption=3, PowerLoss=4, Hotplug=5 };
enum class StressTestState : uint32_t { Idle=0, Running=1, Paused=2, Completed=3, Failed=4, Cancelled=5 };

struct VirtualDeviceConfig {
    VirtualDeviceType type{VirtualDeviceType::Generic};
    std::string deviceId;
    uint32_t connectTimeMs{10};
    uint32_t disconnectProbability{0};
    uint32_t failureProbability{0};
    FailureMode failureMode{FailureMode::None};
};

struct StressTestConfig {
    uint32_t deviceCount{10};
    uint32_t parallelSessions{5};
    uint32_t parallelJobs{3};
    uint32_t operationsPerDevice{10};
    uint32_t connectDisconnectCycles{5};
    uint32_t randomFailureRate{0};
    uint32_t timeoutMs{5000};
    bool enableHotplug{false};
    bool enablePowerLoss{false};
    bool enableCorruption{false};
};

struct StressTestResult {
    uint32_t totalOperations{0};
    uint32_t succeeded{0};
    uint32_t failed{0};
    uint32_t timedOut{0};
    uint32_t recovered{0};
    uint64_t totalDurationMs{0};
    StressTestState state{StressTestState::Idle};
    std::vector<std::string> errors;
};

class VirtualStressEnvironment;

} }
