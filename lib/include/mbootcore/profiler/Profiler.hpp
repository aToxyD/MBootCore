#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/profiler/ProfilerTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace profiler {

class Profiler {
public:
    Profiler();
    ~Profiler();
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    Profiler(Profiler&&) noexcept;
    Profiler& operator=(Profiler&&) noexcept;

    void begin(const std::string& name);
    void end(const std::string& name);
    Result<ProfilerMeasurement> measurement(const std::string& name) const;
    Result<PerformanceSnapshot> snapshot() const;
    Result<PercentileInfo> percentiles(const std::string& name) const;
    Result<std::vector<HistogramBin>> histogram(const std::string& name, uint32_t bins = 10) const;
    Result<std::string> exportJson() const;
    Result<std::string> exportCsv() const;
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class ScopeProfiler {
public:
    ScopeProfiler(Profiler& profiler, const std::string& name);
    ~ScopeProfiler();
private:
    Profiler& m_profiler;
    std::string m_name;
};

class PerformanceCounter {
public:
    PerformanceCounter();
    void increment(int64_t delta = 1);
    void decrement(int64_t delta = 1);
    int64_t value() const;
    void reset();
    Result<ProfilerMeasurement> measure() const;
private:
    int64_t m_value{0};
};

} }
