#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mbootcore/benchmark/BenchmarkTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace benchmark {

class BenchmarkRunner {
public:
    BenchmarkRunner();
    ~BenchmarkRunner();
    BenchmarkRunner(const BenchmarkRunner&) = delete;
    BenchmarkRunner& operator=(const BenchmarkRunner&) = delete;
    BenchmarkRunner(BenchmarkRunner&&) noexcept;
    BenchmarkRunner& operator=(BenchmarkRunner&&) noexcept;

    using BenchmarkFunc = std::function<Result<BenchmarkResult>()>;

    Result<void> registerBenchmark(const std::string& name, BenchmarkFunc func);
    Result<void> runAll();
    Result<void> runSingle(const std::string& name);
    Result<BenchmarkReport> report() const;
    Result<std::string> exportJson() const;
    Result<void> clear();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
