#pragma once

#include <mbootcore/runtime/RuntimeStatistics.hpp>

#include <atomic>
#include <chrono>
#include <mutex>

namespace mbootcore {
namespace runtime {

struct RuntimeState {
    mutable std::mutex opMutex;
    mutable std::mutex statsMutex;
    RuntimeStatistics stats;

    std::atomic<bool> initialized{false};
    std::atomic<bool> connected{false};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> paused{false};

    std::chrono::steady_clock::time_point startTime;
};

} // namespace runtime
} // namespace mbootcore
