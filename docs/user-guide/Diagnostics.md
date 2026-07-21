# Diagnostics & Instrumentation

## Overview

MBootCore provides a comprehensive set of diagnostic and instrumentation components for profiling, memory tracking, stress testing, fault injection, and telemetry collection.

## Performance Profiler

The `PerformanceProfiler` provides RAII-based scoped profiling with wall-clock and CPU time measurement:

```cpp
#include <mbootcore/profiler/PerformanceProfiler.hpp>

mbootcore::PerformanceProfiler profiler;

{
    auto scope = profiler.scope("sahara-handshake");
    // ... operations ...
} // scope destructor records elapsed time

auto report = profiler.report();
```

Key features:
- Automatic timing on scope exit
- Wall-clock and CPU time measurement
- Named profiling regions
- Aggregated statistics (min, max, avg, count, total)

## Memory Tracker

The `MemoryTracker` hooks into the platform CRT allocator to track allocations and verify cleanup:

```cpp
#include <mbootcore/memory/MemoryTracker.hpp>

mbootcore::MemoryTracker tracker;
tracker.beginTracking();

// ... operations under test ...

auto report = tracker.endTracking();
// report.leakCount == 0 for clean code
```

Key features:
- Platform CRT hook-based allocation tracking
- Detects memory leaks and double-frees
- Thread-safe concurrent tracking
- Allocation/free pairing verification

## Stress Test Framework

The `StressTestFramework` validates system behavior under extreme workloads:

```cpp
#include <mbootcore/stress/StressTestFramework.hpp>

mbootcore::StressTestFramework stress;
stress.concurrentSessions(100);
stress.rapidConnectDisconnect(1000);
```

Key capabilities:
- Concurrent session simulation (100+ sessions)
- Rapid connect/disconnect cycling (1000+ iterations)
- Memory pressure tests
- Long-running endurance tests

## Fault Injection Engine

The `FaultInjectionEngine` injects configurable faults for testing error handling paths:

```cpp
#include <mbootcore/faults/FaultInjectionEngine.hpp>

mbootcore::FaultInjectionEngine faults;
faults.setProbability(0.2);          // 20% fault probability
faults.setTypeFilter(FaultType::Nak | FaultType::Timeout);
```

Key features:
- Configurable fault probability
- Type-based fault filtering (NAK, timeout, disconnect, corrupt data)
- Random fault injection
- Integration with virtual device infrastructure

## Telemetry Collector

The `TelemetryCollector` aggregates runtime metrics with thread-safe snapshot capabilities:

```cpp
#include <mbootcore/telemetry/TelemetryCollector.hpp>

mbootcore::TelemetryCollector telemetry;

// Counter: count events
telemetry.incrementCounter("packets.sent");
telemetry.incrementCounter("packets.received", 10);

// Duration: record operation times
telemetry.recordDuration("sahara.handshake", 1.5);

// Gauge: track current values
telemetry.setGauge("active.sessions", 5);

// Snapshot
auto snapshot = telemetry.snapshot();
```

Metric types:
- **Counter**: monotonically increasing count (packets, bytes, errors)
- **Duration**: operation timing with statistics
- **Gauge**: point-in-time value (active sessions, queue depth)

## Diagnostics Manager

The `DiagnosticsManager` provides a unified interface for system health analysis:

```cpp
#include <mbootcore/diagnostics/DiagnosticsManager.hpp>

mbootcore::DiagnosticsManager diag;

// Run all registered health checks
auto report = diag.runFullDiagnostics();

// Run checks for a specific category
auto transportReport = diag.runCategory(
    mbootcore::diagnostics::DiagnosticCategory::Transport);

// Run a single check by ID
auto singleReport = diag.runCheck("runtime-health");

// Get actionable recommendations from a report
auto recommendations = diag.getRecommendations(report);

// Session tracking
diag.setSessionId("device-001");
```

Components:
- `DiagnosticsManager` — orchestration and coordination
- `DiagnosticCheck` — abstract base for configurable health checks (8 built-in: Runtime, Memory, Transport, Pipeline, Plugin, DSP, Configuration, Deadlock)
- `DiagnosticSession` — session-scoped diagnostic tracking
- `DiagnosticReport` — structured report with issues, recommendations, and summary

## Benchmark Runner

The `BenchmarkRunner` provides registration and execution of benchmark cases:

```cpp
#include <mbootcore/benchmark/BenchmarkRunner.hpp>

mbootcore::BenchmarkRunner bench;
bench.registerBenchmark("sahara.serialize", []() {
    // ... benchmark code ...
});
bench.executeAll();
auto json = bench.exportToJson();
```

Key features:
- Named benchmark registration
- Sequential or filtered execution
- JSON export for trend analysis
- Integration with test framework (17 benchmark cases, <25ms total)
