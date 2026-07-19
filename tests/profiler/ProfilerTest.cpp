#include <catch2/catch_test_macros.hpp>
#include <mbootcore/profiler/Profiler.hpp>
#include <thread>

using namespace mbootcore::profiler;

TEST_CASE("ProfilerTypesTest", "[profiler]") {
    SECTION("profilerMeasurementDefaults") {
        ProfilerMeasurement m;
        REQUIRE(m.name.empty());
        REQUIRE(m.durationNs == 0);
        REQUIRE(m.cpuCycles == 0);
        REQUIRE(m.memoryBytes == 0);
        REQUIRE(m.throughput == 0);
        REQUIRE(m.latencyNs == 0);
    }

    SECTION("performanceSnapshotDefaults") {
        PerformanceSnapshot s;
        REQUIRE(s.label.empty());
        REQUIRE(s.measurements.empty());
        REQUIRE(s.aggregates.empty());
    }

    SECTION("histogramBinDefaults") {
        HistogramBin b;
        REQUIRE(b.rangeStart == 0.0);
        REQUIRE(b.rangeEnd == 0.0);
        REQUIRE(b.count == 0u);
    }

    SECTION("percentileInfoDefaults") {
        PercentileInfo p;
        REQUIRE(p.p50 == 0.0);
        REQUIRE(p.p90 == 0.0);
        REQUIRE(p.p95 == 0.0);
        REQUIRE(p.p99 == 0.0);
        REQUIRE(p.p999 == 0.0);
        REQUIRE(p.min == 0.0);
        REQUIRE(p.max == 0.0);
        REQUIRE(p.avg == 0.0);
    }
}

TEST_CASE("ProfilerTest", "[profiler]") {
    SECTION("beginEndLifetime") {
        Profiler p;
        p.begin("test");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        p.end("test");

        const auto m = p.measurement("test");
        REQUIRE(m.isOk());
        REQUIRE(m.value().durationNs > 0);
        REQUIRE(m.value().name == std::string("test"));
    }

    SECTION("multipleMeasurements") {
        Profiler p;
        p.begin("a"); p.end("a");
        p.begin("a"); p.end("a");

        const auto m = p.measurement("a");
        REQUIRE(m.isOk());
        REQUIRE(m.value().durationNs > 0);
    }

    SECTION("measurementNotFound") {
        Profiler p;
        const auto m = p.measurement("nonexistent");
        REQUIRE(m.isError());
    }

    SECTION("snapshotWithAggregates") {
        Profiler p;
        for (int i = 0; i < 5; ++i) {
            p.begin("op");
            p.end("op");
        }

        const auto snap = p.snapshot();
        REQUIRE(snap.isOk());
        REQUIRE(snap.value().measurements.size() >= 5);
        REQUIRE(snap.value().aggregates.find("op.avg") != snap.value().aggregates.end());
        REQUIRE(snap.value().aggregates.find("op.min") != snap.value().aggregates.end());
        REQUIRE(snap.value().aggregates.find("op.max") != snap.value().aggregates.end());
        REQUIRE(snap.value().aggregates.find("op.count") != snap.value().aggregates.end());
        REQUIRE(static_cast<int>(snap.value().aggregates.at("op.count")) == 5);
    }

    SECTION("percentilesComputed") {
        Profiler p;
        p.begin("latency");
        p.end("latency");

        const auto perc = p.percentiles("latency");
        REQUIRE(perc.isOk());
        REQUIRE(perc.value().p50 >= 0);
        REQUIRE(perc.value().p90 >= 0);
        REQUIRE(perc.value().p95 >= 0);
        REQUIRE(perc.value().p99 >= 0);
        REQUIRE(perc.value().p999 >= 0);
    }

    SECTION("percentilesNotFound") {
        Profiler p;
        const auto perc = p.percentiles("noop");
        REQUIRE(perc.isError());
    }

    SECTION("histogramComputed") {
        Profiler p;
        p.begin("h");
        p.end("h");

        const auto h = p.histogram("h", 5);
        REQUIRE(h.isOk());
        REQUIRE(h.value().size() == size_t{5});
    }

    SECTION("histogramNotFound") {
        Profiler p;
        const auto h = p.histogram("missing");
        REQUIRE(h.isError());
    }

    SECTION("resetClearsAll") {
        Profiler p;
        p.begin("x");
        p.end("x");
        p.reset();

        const auto m = p.measurement("x");
        REQUIRE(m.isError());
    }

    SECTION("beginWithoutEndIgnored") {
        Profiler p;
        p.begin("orphan");
        const auto m = p.measurement("orphan");
        REQUIRE(m.isError());
    }
}

TEST_CASE("ScopeProfilerTest", "[profiler]") {
    SECTION("scopeMeasuresLifetime") {
        Profiler p;
        {
            ScopeProfiler sp(p, "scope");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        const auto m = p.measurement("scope");
        REQUIRE(m.isOk());
        REQUIRE(m.value().durationNs > 0);
    }

    SECTION("multipleScopes") {
        Profiler p;
        {
            ScopeProfiler sp(p, "s1");
        }
        {
            ScopeProfiler sp(p, "s2");
        }
        REQUIRE(p.measurement("s1").isOk());
        REQUIRE(p.measurement("s2").isOk());
    }
}

TEST_CASE("PerformanceCounterTest", "[profiler]") {
    SECTION("initialValueZero") {
        PerformanceCounter c;
        REQUIRE(c.value() == 0);
    }

    SECTION("incrementIncreases") {
        PerformanceCounter c;
        c.increment();
        REQUIRE(c.value() == 1);
        c.increment(5);
        REQUIRE(c.value() == 6);
    }

    SECTION("decrementDecreases") {
        PerformanceCounter c;
        c.increment(10);
        c.decrement(3);
        REQUIRE(c.value() == 7);
    }

    SECTION("resetClears") {
        PerformanceCounter c;
        c.increment(100);
        c.reset();
        REQUIRE(c.value() == 0);
    }

    SECTION("measureReturnsValue") {
        PerformanceCounter c;
        c.increment(42);
        const auto m = c.measure();
        REQUIRE(m.isOk());
        REQUIRE(m.value().memoryBytes == 42);
    }
}

TEST_CASE("ExportTest", "[profiler]") {
    SECTION("exportJsonFormat") {
        Profiler p;
        p.begin("json_test");
        p.end("json_test");

        const auto json = p.exportJson();
        REQUIRE(json.isOk());
        const auto& s = json.value();
        REQUIRE(s.find("\"name\"") != std::string::npos);
        REQUIRE(s.find("json_test") != std::string::npos);
        REQUIRE(s.find("\"durationNs\"") != std::string::npos);
    }

    SECTION("exportCsvFormat") {
        Profiler p;
        p.begin("csv_test");
        p.end("csv_test");

        const auto csv = p.exportCsv();
        REQUIRE(csv.isOk());
        const auto& s = csv.value();
        REQUIRE(s.find("name,durationNs") != std::string::npos);
        REQUIRE(s.find("csv_test") != std::string::npos);
    }

    SECTION("exportEmptyProfiler") {
        Profiler p;
        const auto json = p.exportJson();
        REQUIRE(json.isOk());
        const auto csv = p.exportCsv();
        REQUIRE(csv.isOk());
    }
}
