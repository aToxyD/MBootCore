#include <catch2/catch_test_macros.hpp>

#include <mbootcore/benchmark/BenchmarkTypes.hpp>
#include <mbootcore/benchmark/BenchmarkRunner.hpp>
#include <mbootcore/domain/Error.hpp>
#include <algorithm>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <numeric>

using namespace mbootcore;
using namespace mbootcore::benchmark;

static Result<BenchmarkResult> makeSortBenchmark() {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), std::mt19937{42});
    std::sort(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    BenchmarkResult r;
    r.name = "sort_bench";
    r.durationNs = ns;
    r.operations = 10000;
    r.opsPerSecond = 1e9 / (ns / 10000.0);
    return r;
}

static Result<BenchmarkResult> makeHashBenchmark() {
    auto start = std::chrono::high_resolution_clock::now();
    std::hash<std::string> hasher;
    size_t h = 0;
    for (int i = 0; i < 1000; ++i) {
        h ^= hasher("benchmark_test_string_" + std::to_string(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    BenchmarkResult r;
    r.name = "hash_bench";
    r.durationNs = ns;
    r.operations = 1000;
    r.opsPerSecond = 1e9 / (ns / 1000.0);
    (void)h;
    return r;
}

static Result<BenchmarkResult> makeParseBenchmark() {
    auto start = std::chrono::high_resolution_clock::now();
    std::string input = "key1=value1;key2=value2;key3=value3;key4=value4";
    std::map<std::string, std::string> parsed;
    std::istringstream stream(input);
    std::string token;
    while (std::getline(stream, token, ';')) {
        auto eq = token.find('=');
        if (eq != std::string::npos) {
            parsed[token.substr(0, eq)] = token.substr(eq + 1);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    BenchmarkResult r;
    r.name = "parse_bench";
    r.durationNs = ns;
    r.operations = static_cast<int64_t>(parsed.size());
    r.opsPerSecond = 1e9 / (ns / std::max(1.0, static_cast<double>(parsed.size())));
    return r;
}

TEST_CASE("BenchmarkTest", "[benchmark]") {

    SECTION("benchmarkResultDefaults") {
        BenchmarkResult r;
        REQUIRE(r.name.empty());
        REQUIRE(r.durationNs == 0);
        REQUIRE(r.operations == 0);
        REQUIRE(r.opsPerSecond == 0.0);
        REQUIRE(r.throughputMBs == 0.0);
        REQUIRE(r.memoryBytes == size_t{0});
        REQUIRE(r.metrics.empty());
    }

    SECTION("benchmarkReportDefaults") {
        BenchmarkReport r;
        REQUIRE(r.suite.empty());
        REQUIRE(r.timestamp.empty());
        REQUIRE(r.results.empty());
        REQUIRE(r.averages.empty());
        REQUIRE(r.totalDurationMs == 0.0);
    }

    SECTION("benchmarkResultWithValues") {
        BenchmarkResult r;
        r.name = "sort_test";
        r.durationNs = 1000000;
        r.operations = 1000;
        r.opsPerSecond = 1e9;
        r.throughputMBs = 500.0;
        r.memoryBytes = 4096;
        r.metrics["items"] = 1000.0;

        REQUIRE(r.name == std::string("sort_test"));
        REQUIRE(r.durationNs == int64_t{1000000});
        REQUIRE(r.operations == int64_t{1000});
        REQUIRE(r.opsPerSecond == 1e9);
        REQUIRE(r.throughputMBs == 500.0);
        REQUIRE(r.memoryBytes == size_t{4096});
        REQUIRE(r.metrics.at("items") == 1000.0);
    }

    SECTION("registerBenchmarks") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("sort", makeSortBenchmark).isOk());
        REQUIRE(runner.registerBenchmark("hash", makeHashBenchmark).isOk());
        REQUIRE(runner.registerBenchmark("parse", makeParseBenchmark).isOk());
    }

    SECTION("registerDuplicateFails") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("test", makeSortBenchmark).isOk());
        REQUIRE(runner.registerBenchmark("test", makeSortBenchmark).isError());
    }

    SECTION("registerEmptyNameFails") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("", makeSortBenchmark).isError());
    }

    SECTION("runAllBenchmarks") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("sort", makeSortBenchmark).isOk());
        REQUIRE(runner.registerBenchmark("hash", makeHashBenchmark).isOk());
        REQUIRE(runner.runAll().isOk());

        auto rep = runner.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().results.size() == size_t{2});
    }

    SECTION("runSingleBenchmark") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("sort", makeSortBenchmark).isOk());
        REQUIRE(runner.registerBenchmark("hash", makeHashBenchmark).isOk());
        REQUIRE(runner.runSingle("sort").isOk());

        auto rep = runner.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().results.size() == size_t{1});
        REQUIRE(rep.value().results[0].name == std::string("sort_bench"));
    }

    SECTION("runNonexistentSingleFails") {
        BenchmarkRunner runner;
        REQUIRE(runner.runSingle("nonexistent").isError());
    }

    SECTION("reportWithResults") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("hash", makeHashBenchmark).isOk());
        REQUIRE(runner.runAll().isOk());

        auto rep = runner.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().suite == std::string("MBootCore Benchmark Suite"));
        REQUIRE(!rep.value().timestamp.empty());
        REQUIRE(rep.value().results.size() == size_t{1});
        REQUIRE(rep.value().results[0].durationNs > 0);
        REQUIRE(rep.value().results[0].opsPerSecond > 0);
        REQUIRE(rep.value().totalDurationMs > 0);
    }

    SECTION("emptyReport") {
        BenchmarkRunner runner;
        auto rep = runner.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().results.empty());
        REQUIRE(rep.value().totalDurationMs == 0.0);
    }

    SECTION("jsonExport") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("sort", makeSortBenchmark).isOk());
        REQUIRE(runner.runAll().isOk());

        auto json = runner.exportJson();
        REQUIRE(json.isOk());
        REQUIRE(!json.value().empty());
        REQUIRE(json.value().find("suite") != std::string::npos);
        REQUIRE(json.value().find("sort_bench") != std::string::npos);
        REQUIRE(json.value().find("durationNs") != std::string::npos);
        REQUIRE(json.value().find("averages") != std::string::npos);
    }

    SECTION("jsonExportEmpty") {
        BenchmarkRunner runner;
        auto json = runner.exportJson();
        REQUIRE(json.isOk());
        REQUIRE(json.value().find("\"results\": [") != std::string::npos);
    }

    SECTION("clearRunner") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("sort", makeSortBenchmark).isOk());
        REQUIRE(runner.runAll().isOk());
        REQUIRE(runner.clear().isOk());

        auto rep = runner.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().results.empty());
    }

    SECTION("registerAfterClear") {
        BenchmarkRunner runner;
        REQUIRE(runner.registerBenchmark("sort", makeSortBenchmark).isOk());
        REQUIRE(runner.runAll().isOk());
        REQUIRE(runner.clear().isOk());
        REQUIRE(runner.registerBenchmark("hash", makeHashBenchmark).isOk());

        auto rep = runner.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().results.empty());

        REQUIRE(runner.runAll().isOk());
        rep = runner.report();
        REQUIRE(rep.value().results.size() == size_t{1});
        REQUIRE(rep.value().results[0].name == std::string("hash_bench"));
    }
}
