# Testing Guide

## Overview

All MBootCore tests pass with zero failures. Tests use virtual devices and mocks — no physical hardware is required.

## Test Executables

### Protocol Tests

| Test | File | Coverage |
|------|------|----------|
| `types_test` | `tests/domain/TypesTest.cpp` | ProtocolVersion, Result<T>, ErrorCode |
| `state_machine_test` | `tests/core/state/StateMachineTest.cpp` | GenericStateMachine transitions, reset |

**Sahara (10 tests):**

| Test | Coverage |
|------|----------|
| `sahara_packets_test` | All 19 packet types |
| `sahara_serializer_test` | Packet serialization |
| `sahara_parser_test` | Packet parsing |
| `sahara_state_machine_test` | State transitions, full flow |
| `sahara_protocol_test` | 15 integration tests (V2/V3 handshake, upload, reset, cancel, errors) |
| `sahara_golden_vectors_test` | All 19 packets round-trip + raw bytes |
| `sahara_fuzz_test` | 500+ random/corrupt inputs |
| `sahara_virtual_test` | 10 scenario tests |

**Firehose (3 tests):**

| Test | Coverage |
|------|----------|
| `firehose_golden_vectors_test` | All 13 commands round-trip |
| `firehose_fuzz_test` | Random XML, malformed, nested |
| `firehose_virtual_test` | 12 integration tests |

### Infrastructure Tests

| Test | Coverage |
|------|----------|
| `sahara_virtual_test` | 10 Sahara scenario tests |
| `generic_flash_test` | 20 tests (capabilities, adapters, flash operations, cancel, progress, pipeline) |
| `loader_framework_test` | Loader search, matching, validation |
| `elf_engine_test` | 34 tests (parser, validator, memory image, virtual programmer) |
| `boot_pipeline_test` | 15 tests (all stages, recovery, cancellation, factory) |

### GPT Tests (4 tests)

| Test | Coverage |
|------|----------|
| `gpt_models_test` | 19 tests (Guid, entries, header, types) |
| `gpt_parser_test` | 17 tests (valid, invalid, CRC, overlap) |
| `gpt_writer_test` | 10 tests (primary, backup, CRC consistency) |
| `gpt_manager_test` | 20 tests (all high-level operations, recovery) |

### Service Layer Tests

| Test | Coverage |
|------|----------|
| `discovery_test` | 57 tests (detection, negotiation, registry, virtual devices, hotplug) |
| `session_test` | ~80 tests (lifecycle, operations, observers, cancellation, statistics, factory, manager) |
| `plugin_test` | 71 tests (types, lifecycle, dependency, batch, query, protocol/vendor plugins) |
| `job_engine_test` | 101 tests (jobs, pipeline, scheduler, history, progress) |
| `firmware_package_test` | 115 tests (types, package, readers, validator, resolver, executor) |

### Diagnostics, Profiler & Security Tests (14 tests)

| Test | Description |
|------|-------------|
| `diagnostics_test` | DiagnosticsManager, DiagnosticCheck, DiagnosticSession, DiagnosticReport |
| `profiler_test` | PerformanceProfiler scoped profiling |
| `memory_tracker_test` | MemoryTracker allocation tracking |
| `stress_test` | StressTestFramework concurrent sessions + rapid cycles |
| `fault_test` | FaultInjectionEngine injection and filtering |
| `recovery_test` | RecoveryStrategy rule validation |
| `security_test` | SecurityManager, PermissionSet, InMemoryStorage |
| `config_test` | ConfigManager JSON/YAML/INI parsing |
| `telemetry_test` | TelemetryTypes, TelemetryCollector |
| `logging_test` | StructuredLogger JSON-line output |
| `benchmark_test` | BenchmarkRunner registration, execution, JSON export |
| `compatibility_test` | Cross-platform compatibility matrix |
| `fuzz_test` | Fuzz test harness (500+ random inputs) |
| `longrunning_test` | Long-running stress endurance tests |

### GUI Tests (18 tests)

Covering all 15 widget categories: framework, discovery, session, firmware, flash, GPT, workflow, job, plugin, vendor, transport, logs, settings, diagnostics, devtools, theme, l10n, integration.

### SDK Tests (17 tests)

Covering Plugin SDK, Vendor SDK, templates, compatibility, diagnostics.

### Hardware Tests (3 tests)

Auto-skip when hardware absent. Qualcomm EDL detection; MediaTek BROM detection (scaffold); UNISOC detection (scaffold).

## Test Infrastructure

### Mocks

- **MockTransport** — Programmable transport with queued read data, write recording, error simulation
- **MockLogger** — Records all log messages for verification
- **MockFlashDevice** — Simulates flash storage with read/write/erase tracking

### Virtual Devices

- **VirtualSaharaDevice** — Simulates Sahara device behavior (V2/V3 handshake, upload, reset, errors)
- **VirtualFirehoseDevice** — Pre-queues XML responses for Firehose testing
- **VirtualFlashDevice** — Generic flash storage simulation

## Running Tests

```powershell
cd build

# Run all tests
ctest --output-on-failure -j4

# Run protocol tests
ctest -R "sahara|firehose" --output-on-failure

# Run GUI tests
ctest -R "^studio_" --output-on-failure

# Run hardware tests (auto-skips if no device)
ctest -L Hardware --output-on-failure

# Run specific test
.\sahara_protocol_test.exe -o ,txt
```
