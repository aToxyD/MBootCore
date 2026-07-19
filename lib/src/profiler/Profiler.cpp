#include <mbootcore/profiler/Profiler.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <nlohmann/json.hpp>

namespace mbootcore { namespace profiler {

struct Profiler::Impl {
    std::map<std::string, std::vector<ProfilerMeasurement>> history;
    std::map<std::string, std::chrono::steady_clock::time_point> activeTimers;
    static constexpr size_t maxHistoryPerName = 1000;

    void addMeasurement(const std::string& name, ProfilerMeasurement m) {
        auto& vec = history[name];
        vec.push_back(std::move(m));
        if (vec.size() > maxHistoryPerName) {
            vec.erase(vec.begin(), vec.begin() + static_cast<ptrdiff_t>(vec.size() - maxHistoryPerName));
        }
    }
};

Profiler::Profiler() : m_impl(std::make_unique<Impl>()) {}
Profiler::~Profiler() = default;
Profiler::Profiler(Profiler&&) noexcept = default;
Profiler& Profiler::operator=(Profiler&&) noexcept = default;

void Profiler::begin(const std::string& name) {
    m_impl->activeTimers[name] = std::chrono::steady_clock::now();
}

void Profiler::end(const std::string& name) {
    const auto endTime = std::chrono::steady_clock::now();
    const auto it = m_impl->activeTimers.find(name);
    if (it == m_impl->activeTimers.end()) {
        return;
    }
    const auto startTime = it->second;
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();

    ProfilerMeasurement m;
    m.name = name;
    m.durationNs = duration;
    m.timestamp = endTime;

    m_impl->addMeasurement(name, std::move(m));
    m_impl->activeTimers.erase(it);
}

Result<ProfilerMeasurement> Profiler::measurement(const std::string& name) const {
    const auto it = m_impl->history.find(name);
    if (it == m_impl->history.end() || it->second.empty()) {
        return ErrorCode::InvalidArgument;
    }
    return it->second.back();
}

Result<PerformanceSnapshot> Profiler::snapshot() const {
    PerformanceSnapshot snap;
    snap.timestamp = std::chrono::steady_clock::now();

    for (const auto& [name, measurements] : m_impl->history) {
        if (measurements.empty()) continue;

        for (const auto& m : measurements) {
            snap.measurements.push_back(m);
        }

        double sum = 0;
        double minV = static_cast<double>(measurements.front().durationNs);
        double maxV = static_cast<double>(measurements.front().durationNs);

        for (const auto& m : measurements) {
            const double d = static_cast<double>(m.durationNs);
            sum += d;
            if (d < minV) minV = d;
            if (d > maxV) maxV = d;
        }

        const double avg = sum / static_cast<double>(measurements.size());
        const double count = static_cast<double>(measurements.size());

        snap.aggregates[name + ".avg"] = avg;
        snap.aggregates[name + ".min"] = minV;
        snap.aggregates[name + ".max"] = maxV;
        snap.aggregates[name + ".count"] = count;
    }

    return snap;
}

Result<PercentileInfo> Profiler::percentiles(const std::string& name) const {
    const auto it = m_impl->history.find(name);
    if (it == m_impl->history.end() || it->second.empty()) {
        return ErrorCode::InvalidArgument;
    }

    std::vector<double> durations;
    durations.reserve(it->second.size());
    double sum = 0;
    for (const auto& m : it->second) {
        const double d = static_cast<double>(m.durationNs);
        durations.push_back(d);
        sum += d;
    }

    std::sort(durations.begin(), durations.end());

    PercentileInfo info;
    info.min = durations.front();
    info.max = durations.back();
    info.avg = sum / static_cast<double>(durations.size());

    const auto percentile = [&](double p) -> double {
        if (durations.empty()) return 0;
        const double idx = p * static_cast<double>(durations.size() - 1);
        const size_t lo = static_cast<size_t>(std::floor(idx));
        const size_t hi = static_cast<size_t>(std::ceil(idx));
        if (lo == hi || hi >= durations.size()) return durations[lo];
        const double frac = idx - static_cast<double>(lo);
        return durations[lo] * (1.0 - frac) + durations[hi] * frac;
    };

    info.p50 = percentile(0.50);
    info.p90 = percentile(0.90);
    info.p95 = percentile(0.95);
    info.p99 = percentile(0.99);
    info.p999 = percentile(0.999);

    return info;
}

Result<std::vector<HistogramBin>> Profiler::histogram(const std::string& name, uint32_t bins) const {
    const auto it = m_impl->history.find(name);
    if (it == m_impl->history.end() || it->second.empty()) {
        return ErrorCode::InvalidArgument;
    }

    const auto& measurements = it->second;
    if (bins == 0) bins = 1;

    double minV = static_cast<double>(measurements.front().durationNs);
    double maxV = static_cast<double>(measurements.front().durationNs);

    for (const auto& m : measurements) {
        const double d = static_cast<double>(m.durationNs);
        if (d < minV) minV = d;
        if (d > maxV) maxV = d;
    }

    const double range = maxV - minV;
    const double binWidth = (range > 0) ? range / static_cast<double>(bins) : 1.0;

    std::vector<HistogramBin> result(bins);
    for (uint32_t i = 0; i < bins; ++i) {
        result[i].rangeStart = minV + static_cast<double>(i) * binWidth;
        result[i].rangeEnd = result[i].rangeStart + binWidth;
        result[i].count = 0;
    }

    for (const auto& m : measurements) {
        const double d = static_cast<double>(m.durationNs);
        uint32_t idx = static_cast<uint32_t>((d - minV) / binWidth);
        if (idx >= bins) idx = bins - 1;
        result[idx].count++;
    }

    return result;
}

Result<std::string> Profiler::exportJson() const {
    try {
        nlohmann::json j;
        nlohmann::json measurements = nlohmann::json::array();
        for (const auto& [name, mvec] : m_impl->history) {
            for (const auto& m : mvec) {
                nlohmann::json entry;
                entry["name"] = m.name;
                entry["durationNs"] = m.durationNs;
                entry["cpuCycles"] = m.cpuCycles;
                entry["memoryBytes"] = m.memoryBytes;
                entry["throughput"] = m.throughput;
                entry["latencyNs"] = m.latencyNs;
                measurements.push_back(std::move(entry));
            }
        }
        j["measurements"] = std::move(measurements);
        return j.dump(2);
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<std::string> Profiler::exportCsv() const {
    std::ostringstream ss;
    ss << "name,durationNs,cpuCycles,memoryBytes,throughput,latencyNs\n";
    for (const auto& [name, measurements] : m_impl->history) {
        for (const auto& m : measurements) {
            ss << m.name << ","
               << m.durationNs << ","
               << m.cpuCycles << ","
               << m.memoryBytes << ","
               << m.throughput << ","
               << m.latencyNs << "\n";
        }
    }
    return ss.str();
}

void Profiler::reset() {
    m_impl->history.clear();
    m_impl->activeTimers.clear();
}

ScopeProfiler::ScopeProfiler(Profiler& profiler, const std::string& name)
    : m_profiler(profiler), m_name(name) {
    m_profiler.begin(m_name);
}

ScopeProfiler::~ScopeProfiler() {
    m_profiler.end(m_name);
}

PerformanceCounter::PerformanceCounter() : m_value(0) {}

void PerformanceCounter::increment(int64_t delta) {
    m_value += delta;
}

void PerformanceCounter::decrement(int64_t delta) {
    m_value -= delta;
}

int64_t PerformanceCounter::value() const {
    return m_value;
}

void PerformanceCounter::reset() {
    m_value = 0;
}

Result<ProfilerMeasurement> PerformanceCounter::measure() const {
    ProfilerMeasurement m;
    m.name = "counter";
    m.memoryBytes = static_cast<int64_t>(m_value);
    return m;
}

} }
