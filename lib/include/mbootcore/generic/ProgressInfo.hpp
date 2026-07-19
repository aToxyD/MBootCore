#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <chrono>

namespace mbootcore {

struct ProgressInfo {
    uint64_t totalBytes{0};
    uint64_t transferredBytes{0};
    double speedBps{0.0};
    double estimatedSeconds{0.0};
    double percentage{0.0};
    std::string currentOperation;
    std::string stage;
    bool cancelable{true};
    bool isIndeterminate{false};
};

using ProgressCallback = std::function<void(const ProgressInfo& info)>;

} // namespace mbootcore
