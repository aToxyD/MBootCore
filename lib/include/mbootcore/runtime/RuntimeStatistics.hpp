#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace mbootcore {
namespace runtime {

struct RuntimeStatistics {
    uint32_t devicesConnected{0};
    uint32_t devicesDisconnected{0};
    uint32_t jobsExecuted{0};
    uint32_t workflowsExecuted{0};

    double averageFlashSpeedBps{0.0};
    double averageReadSpeedBps{0.0};
    double averageWriteSpeedBps{0.0};

    uint32_t packagesInstalled{0};
    uint32_t packagesFlashed{0};
    uint32_t pluginsLoaded{0};
    uint32_t vendorsLoaded{0};

    double averageWorkflowTimeMs{0.0};
    double uptimeSeconds{0.0};

    uint32_t totalErrors{0};
    uint32_t totalRecoveries{0};
    uint32_t totalTimeouts{0};
    uint32_t totalDisconnects{0};
};

struct RuntimeHealth {
    uint32_t activeSessions{0};
    uint32_t activeWorkflows{0};
    uint32_t queuedJobs{0};
    uint32_t loadedPlugins{0};
    uint32_t loadedVendors{0};
    uint32_t connectedDevices{0};
    uint64_t memoryUsageBytes{0};
    uint32_t threadCount{0};
    std::string transportState;
    double uptimeSeconds{0.0};
};

} // namespace runtime
} // namespace mbootcore
