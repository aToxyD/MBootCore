#pragma once

#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/pipeline/BootPipelineConfig.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/job/JobTypes.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace mbootcore {
namespace runtime {

struct LoggingConfig {
    bool enabled{true};
    std::string level{"info"};
    std::string outputFile;
    bool consoleOutput{true};
    bool fileOutput{false};
    size_t maxEntries{10000};
};

struct RuntimeConfig {
    LoggingConfig logging;
    transport::TransportConfig transport;
    session::SessionConfig session;
    pipeline::BootPipelineConfig pipeline;
    workflow::WorkflowOptions workflow;
    job::JobConfig jobs;

    std::chrono::milliseconds discoverTimeout{5000};
    int maxRetries{3};
    bool autoRecovery{true};
    bool enableVendorRuntime{true};
    bool autoLoadPlugins{true};

    std::vector<std::string> pluginSearchPaths;
    std::vector<std::string> firmwareSearchPaths;
    std::vector<std::string> loaderSearchPaths{"./Loaders"};

    bool enableStatistics{true};
    bool enableMonitoring{true};
    size_t monitorMaxSamples{1000};
};

} // namespace runtime
} // namespace mbootcore
