#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace mbootcore { namespace benchmark {

struct BenchmarkResult {
    std::string name;
    int64_t durationNs{0};
    int64_t operations{0};
    double opsPerSecond{0};
    double throughputMBs{0};
    size_t memoryBytes{0};
    std::map<std::string,double> metrics;
};

struct BenchmarkReport {
    std::string suite;
    std::string timestamp;
    std::vector<BenchmarkResult> results;
    std::map<std::string,double> averages;
    double totalDurationMs{0};
};

class BenchmarkRunner;

} }
