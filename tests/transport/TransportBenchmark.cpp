#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/serial/ISerialBackend.hpp>
#include <mbootcore/transport/network/ITcpBackend.hpp>
#include <mbootcore/transport/network/IUdpBackend.hpp>
#include <mbootcore/benchmark/BenchmarkRunner.hpp>
#include <mbootcore/benchmark/BenchmarkTypes.hpp>
#include <mbootcore/domain/Error.hpp>

#include "MockSerialBackend.hpp"
#include "MockTcpBackend.hpp"
#include <MockUdpBackend.hpp>

#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <cstdint>

using namespace mbootcore;
using namespace mbootcore::transport::serial;
using namespace mbootcore::transport::network;
using namespace mbootcore::benchmark;

static std::vector<uint8_t> makePayload(size_t n) {
    std::vector<uint8_t> buf(n);
    for (size_t i = 0; i < n; ++i) buf[i] = static_cast<uint8_t>(i & 0xFF);
    return buf;
}

static int64_t medianNs(std::vector<int64_t>& samples) {
    if (samples.empty()) return 0;
    auto n = samples.size();
    std::nth_element(samples.begin(), samples.begin() + n / 2, samples.end());
    return samples[n / 2];
}

static Result<BenchmarkResult> benchSerialLatency(size_t payloadSize) {
    MockSerialBackend backend;
    backend.setReadData(makePayload(payloadSize));

    auto openRes = backend.open("loop", 115200, 8, 1, "none", "none", 65536);
    if (openRes.isError())
        return openRes.error();

    auto buf = makePayload(payloadSize);
    std::vector<uint8_t> readBuf(payloadSize);
    std::vector<int64_t> samples;
    samples.reserve(100);

    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        backend.write(buf.data(), buf.size(), std::chrono::seconds(1));
        backend.read(readBuf.data(), readBuf.size(), std::chrono::seconds(1));
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        samples.push_back(ns);
    }

    backend.close();

    auto med = medianNs(samples);

    BenchmarkResult r;
    r.name = "serial_mock_latency_" + std::to_string(payloadSize) + "B";
    r.durationNs = med;
    r.operations = 100;
    r.opsPerSecond = 1e9 / std::max<int64_t>(med, 1);
    r.metrics["payloadBytes"] = static_cast<double>(payloadSize);
    return r;
}

static Result<BenchmarkResult> benchSerialLatency64()  { return benchSerialLatency(64); }
static Result<BenchmarkResult> benchSerialLatency512() { return benchSerialLatency(512); }
static Result<BenchmarkResult> benchSerialLatency4K()  { return benchSerialLatency(4096); }
static Result<BenchmarkResult> benchSerialLatency64K() { return benchSerialLatency(65536); }

static Result<BenchmarkResult> benchSerialThroughput() {
    MockSerialBackend backend;
    size_t total = 1024 * 1024;
    auto payload = makePayload(65536);
    backend.setReadData(payload);

    auto openRes = backend.open("loop", 115200, 8, 1, "none", "none", 65536);
    if (openRes.isError())
        return openRes.error();

    std::vector<uint8_t> readBuf(65536);
    size_t written = 0;

    auto start = std::chrono::high_resolution_clock::now();
    while (written < total) {
        backend.write(payload.data(), payload.size(), std::chrono::seconds(1));
        backend.read(readBuf.data(), readBuf.size(), std::chrono::seconds(1));
        written += payload.size();
    }
    auto end = std::chrono::high_resolution_clock::now();

    backend.close();

    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double secs = static_cast<double>(ns) / 1e9;

    BenchmarkResult r;
    r.name = "serial_mock_throughput";
    r.durationNs = ns;
    r.operations = static_cast<int64_t>(total / payload.size());
    r.throughputMBs = (static_cast<double>(total) / (1024.0 * 1024.0)) / secs;
    return r;
}

static Result<BenchmarkResult> benchTcpLatency(size_t payloadSize) {
    MockTcpBackend backend;
    backend.setReadData(makePayload(payloadSize));

    auto openRes = backend.open("127.0.0.1", 12345, true, std::chrono::seconds(1));
    if (openRes.isError())
        return openRes.error();

    auto buf = makePayload(payloadSize);
    std::vector<uint8_t> readBuf(payloadSize);
    std::vector<int64_t> samples;
    samples.reserve(100);

    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        backend.write(buf.data(), buf.size(), std::chrono::seconds(1));
        backend.read(readBuf.data(), readBuf.size(), std::chrono::seconds(1));
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        samples.push_back(ns);
    }

    backend.close();

    auto med = medianNs(samples);

    BenchmarkResult r;
    r.name = "tcp_mock_latency_" + std::to_string(payloadSize) + "B";
    r.durationNs = med;
    r.operations = 100;
    r.opsPerSecond = 1e9 / std::max<int64_t>(med, 1);
    r.metrics["payloadBytes"] = static_cast<double>(payloadSize);
    return r;
}

static Result<BenchmarkResult> benchTcpLatency64()  { return benchTcpLatency(64); }
static Result<BenchmarkResult> benchTcpLatency512() { return benchTcpLatency(512); }
static Result<BenchmarkResult> benchTcpLatency4K()  { return benchTcpLatency(4096); }
static Result<BenchmarkResult> benchTcpLatency64K() { return benchTcpLatency(65536); }

static Result<BenchmarkResult> benchReconnectSerial() {
    MockSerialBackend backend;
    std::vector<int64_t> samples;
    samples.reserve(50);

    for (int i = 0; i < 50; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        auto openRes = backend.open("loop", 115200, 8, 1, "none", "none", 65536);
        if (openRes.isError())
            return openRes.error();
        backend.close();
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        samples.push_back(ns);
    }

    auto med = medianNs(samples);

    BenchmarkResult r;
    r.name = "serial_mock_reconnect";
    r.durationNs = med;
    r.operations = 50;
    r.opsPerSecond = 1e9 / std::max<int64_t>(med, 1);
    return r;
}

static Result<BenchmarkResult> benchReconnectTcp() {
    MockTcpBackend backend;
    std::vector<int64_t> samples;
    samples.reserve(50);

    for (int i = 0; i < 50; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        auto openRes = backend.open("127.0.0.1", 12345, true, std::chrono::seconds(1));
        if (openRes.isError())
            return openRes.error();
        backend.close();
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        samples.push_back(ns);
    }

    auto med = medianNs(samples);

    BenchmarkResult r;
    r.name = "tcp_mock_reconnect";
    r.durationNs = med;
    r.operations = 50;
    r.opsPerSecond = 1e9 / std::max<int64_t>(med, 1);
    return r;
}

static void runSingle(const char* name, BenchmarkRunner::BenchmarkFunc func) {
    BenchmarkRunner runner;
    REQUIRE(runner.registerBenchmark(name, std::move(func)).isOk());
    REQUIRE(runner.runAll().isOk());

    auto rep = runner.report();
    REQUIRE(rep.isOk());
    REQUIRE(!rep.value().results.empty());

    auto json = runner.exportJson();
    REQUIRE(json.isOk());
    REQUIRE(!json.value().empty());
}

TEST_CASE("TransportBenchmark", "[transport]") {
    SECTION("bench_serial_latency_64B")  { runSingle("serial_latency_64",  benchSerialLatency64); }
    SECTION("bench_serial_latency_512B") { runSingle("serial_latency_512", benchSerialLatency512); }
    SECTION("bench_serial_latency_4K")   { runSingle("serial_latency_4K",  benchSerialLatency4K); }
    SECTION("bench_serial_latency_64K")  { runSingle("serial_latency_64K", benchSerialLatency64K); }
    SECTION("bench_serial_throughput")   { runSingle("serial_throughput",  benchSerialThroughput); }
    SECTION("bench_tcp_latency_64B")     { runSingle("tcp_latency_64",     benchTcpLatency64); }
    SECTION("bench_tcp_latency_512B")    { runSingle("tcp_latency_512",    benchTcpLatency512); }
    SECTION("bench_tcp_latency_4K")      { runSingle("tcp_latency_4K",     benchTcpLatency4K); }
    SECTION("bench_tcp_latency_64K")     { runSingle("tcp_latency_64K",    benchTcpLatency64K); }
    SECTION("bench_serial_reconnect")    { runSingle("serial_reconnect",   benchReconnectSerial); }
    SECTION("bench_tcp_reconnect")       { runSingle("tcp_reconnect",      benchReconnectTcp); }
}
