#include <mbootcore/benchmark/BenchmarkRunner.hpp>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <nlohmann/json.hpp>

namespace mbootcore { namespace benchmark {

struct BenchmarkRunner::Impl {
    std::map<std::string, BenchmarkFunc> benchmarks;
    std::vector<BenchmarkResult> results;
};

BenchmarkRunner::BenchmarkRunner()
    : m_impl(std::make_unique<Impl>())
{
}

BenchmarkRunner::~BenchmarkRunner() = default;
BenchmarkRunner::BenchmarkRunner(BenchmarkRunner&&) noexcept = default;
BenchmarkRunner& BenchmarkRunner::operator=(BenchmarkRunner&&) noexcept = default;

Result<void> BenchmarkRunner::registerBenchmark(const std::string& name, BenchmarkFunc func) {
    if (name.empty()) {
        return ErrorCode::InvalidArgument;
    }
    if (m_impl->benchmarks.find(name) != m_impl->benchmarks.end()) {
        return ErrorCode::AlreadyExists;
    }
    m_impl->benchmarks[name] = std::move(func);
    return {};
}

Result<void> BenchmarkRunner::runAll() {
    m_impl->results.clear();
    for (auto& [name, func] : m_impl->benchmarks) {
        auto result = func();
        if (result.isOk()) {
            m_impl->results.push_back(std::move(result.value()));
        } else {
            return result.error();
        }
    }
    return {};
}

Result<void> BenchmarkRunner::runSingle(const std::string& name) {
    auto it = m_impl->benchmarks.find(name);
    if (it == m_impl->benchmarks.end()) {
        return ErrorCode::InvalidArgument;
    }
    auto result = it->second();
    if (result.isOk()) {
        auto existing = std::find_if(m_impl->results.begin(), m_impl->results.end(),
            [&](const BenchmarkResult& r) { return r.name == name; });
        if (existing != m_impl->results.end()) {
            *existing = std::move(result.value());
        } else {
            m_impl->results.push_back(std::move(result.value()));
        }
    } else {
        return result.error();
    }
    return {};
}

Result<BenchmarkReport> BenchmarkRunner::report() const {
    BenchmarkReport report;
    report.suite = "MBootCore Benchmark Suite";
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%SZ");
    report.timestamp = ss.str();
    report.results = m_impl->results;

    double totalMs = 0;
    for (const auto& r : m_impl->results) {
        totalMs += static_cast<double>(r.durationNs) / 1e6;
    }
    report.totalDurationMs = totalMs;

    if (!m_impl->results.empty()) {
        std::map<std::string, std::vector<double>> metricVals;
        for (const auto& r : m_impl->results) {
            metricVals["durationNs"].push_back(static_cast<double>(r.durationNs));
            metricVals["opsPerSecond"].push_back(r.opsPerSecond);
            metricVals["throughputMBs"].push_back(r.throughputMBs);
            for (const auto& [k, v] : r.metrics) {
                metricVals["metric_" + k].push_back(v);
            }
        }
        for (const auto& [key, vals] : metricVals) {
            if (!vals.empty()) {
                double sum = std::accumulate(vals.begin(), vals.end(), 0.0);
                report.averages[key] = sum / static_cast<double>(vals.size());
            }
        }
    }

    return report;
}

Result<std::string> BenchmarkRunner::exportJson() const {
    auto repResult = report();
    if (repResult.isError()) {
        return repResult.error();
    }
    const auto& rep = repResult.value();

    try {
        nlohmann::json j;
        j["suite"] = rep.suite;
        j["timestamp"] = rep.timestamp;
        j["totalDurationMs"] = rep.totalDurationMs;

        nlohmann::json results = nlohmann::json::array();
        for (const auto& r : rep.results) {
            nlohmann::json result;
            result["name"] = r.name;
            result["durationNs"] = r.durationNs;
            result["operations"] = r.operations;
            result["opsPerSecond"] = r.opsPerSecond;
            result["throughputMBs"] = r.throughputMBs;
            result["memoryBytes"] = r.memoryBytes;
            nlohmann::json metrics = nlohmann::json::object();
            for (const auto& [mk, mv] : r.metrics) {
                metrics[mk] = mv;
            }
            result["metrics"] = std::move(metrics);
            results.push_back(std::move(result));
        }
        j["results"] = std::move(results);

        nlohmann::json averages = nlohmann::json::object();
        for (const auto& [ak, av] : rep.averages) {
            averages[ak] = av;
        }
        j["averages"] = std::move(averages);

        return j.dump(2);
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<void> BenchmarkRunner::clear() {
    m_impl->benchmarks.clear();
    m_impl->results.clear();
    return {};
}

} }
