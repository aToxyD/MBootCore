#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <map>

namespace mbootcore { namespace profiler {

struct ProfilerMeasurement {
    std::string name;
    int64_t durationNs{0};
    int64_t cpuCycles{0};
    int64_t memoryBytes{0};
    int64_t throughput{0};
    int64_t latencyNs{0};
    std::chrono::steady_clock::time_point timestamp;
};

struct PerformanceSnapshot {
    std::string label;
    std::vector<ProfilerMeasurement> measurements;
    std::map<std::string,double> aggregates;
    std::chrono::steady_clock::time_point timestamp;
};

struct HistogramBin {
    double rangeStart{0};
    double rangeEnd{0};
    uint32_t count{0};
};

struct PercentileInfo {
    double p50{0};
    double p90{0};
    double p95{0};
    double p99{0};
    double p999{0};
    double min{0};
    double max{0};
    double avg{0};
};

class Profiler;
class ScopeProfiler;
class PerformanceCounter;

} }
