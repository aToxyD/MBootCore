#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mbootcore {
namespace pipeline {

struct BootPipelineConfig {
    int connectTimeoutMs{5000};
    int saharaTimeoutMs{10000};
    int firehoseTimeoutMs{30000};
    int maxConfigureRetries{3};
    int maxStageRetries{2};
    bool enableRecovery{true};
    bool enableProgressReporting{true};
    std::vector<std::string> loaderSearchPaths{"./Loaders"};

    static BootPipelineConfig defaults() {
        return BootPipelineConfig{};
    }
};

} // namespace pipeline
} // namespace mbootcore
