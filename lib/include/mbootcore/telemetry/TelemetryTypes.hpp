#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace mbootcore { namespace telemetry {

enum class TelemetryCategory : uint32_t {
    Performance = 0,
    Error = 1,
    Crash = 2,
    Statistics = 3,
    Usage = 4,
    System = 5
};

enum class TelemetryState : uint32_t {
    Disabled = 0,
    Enabled = 1,
    Paused = 2
};

struct TelemetryEvent {
    std::string id;
    TelemetryCategory category{ TelemetryCategory::Usage };
    std::string name;
    std::map<std::string, std::string> data;
    std::chrono::system_clock::time_point timestamp;
};

struct TelemetryReport {
    std::string sessionId;
    std::vector<TelemetryEvent> events;
    std::map<std::string, uint64_t> eventCounts;
    std::chrono::system_clock::time_point generated;
};

class TelemetryCollector;

} }
