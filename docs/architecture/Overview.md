# MBootCore Architecture

## 1. Goals

MBootCore is a professional C++17 framework for BootROM protocols. It provides
a complete toolchain for discovering devices in download mode, negotiating
protocols, uploading boot programmers, reading and writing flash memory, and
managing GUID partition tables across multiple SoC vendors.

Key goals:

- Clean Architecture with strict layer separation
- Qt-free core library (Qt is only in `apps/mboot-studio/`)
- Native transport backends on all platforms (no Qt transport)
- Vendor-extensible plugin architecture
- Stable public API with predictable error handling

Production support: Qualcomm Sahara + Firehose.
Scaffold implementations: MediaTek BROM, UNISOC FDL.

## 2. Design Principles

| Principle | Application |
|-----------|-------------|
| Clean Architecture | Dependencies point inward only. Inner layers define interfaces; outer layers implement them. |
| Result\<T\> over exceptions | All fallible operations return `Result<T>`. Errors are values, not control flow. No exceptions in core library code. |
| No global state | Zero singletons, zero global variables, zero static state. |
| Composition over Inheritance | Protocols compose interfaces (`IProtocol` + `ITransport` + `ILogger`), not inherit behavior. |
| RAII + Move semantics | All system resources wrapped in RAII handles. Move-only types for shared context. |
| Strong typing | `enum class`, `std::optional`, `std::variant`, `std::string_view`. No raw integers for semantic types. |
| Const correctness | Methods const by default. Immutable after construction where possible. |
| ISP compliance | `IProtocol` does not depend on `ILogger`. `IDevice` does not expose `transport()`. |

## 3. Layer Diagram

```txt
Application Layer        ---  Session, CLI (mboot-cli), GUI (mboot-studio)
    |
Service Layer            ---  Pipeline, Job, Workflow, Plugin
    |
Capability Layer         ---  Discovery, Firmware, GPT, ELF, Loader, Generic Flash
    |
Protocol Layer           ---  Sahara (binary), Firehose (XML), extensible
    |
Transport Layer          ---  USB (WinUSB/LibUSB), Serial (POSIX/Win32), TCP/UDP (POSIX/Win32)
    |
Domain Layer             ---  IProtocol, ITransport, IFlashDevice, Result<T>, ErrorCode, ByteBuffer, ...
```

Each layer depends only on the layer below it. No circular dependencies exist.

## 4. Dependency Rules

- Dependencies point inward only.
- Domain has no platform or protocol dependencies.
- Core has no protocol knowledge.
- Protocols own their serializers and parsers.
- Generic flash layer is protocol-agnostic.
- Pipeline has zero protocol dependencies (stage handlers bridge to implementations).
- Runtime is the sole backend for the GUI.
- Adapters bridge the generic layer to protocol implementations.
- Discovery uses a registry-based design with confidence scoring.
- Capability system replaces `if (vendor)` branching.
- ELF engine is fully standalone with zero protocol dependencies.
- GPT works via `IFlashDevice` only (no protocol headers).
- Plugin SDK has zero protocol headers.
- GUI communicates exclusively through Runtime. No protocol headers in `gui/`.
- Diagnostics are self-contained with zero protocol dependencies.
- Platform selection via `#ifdef` only inside factory `.cpp` files — never in transport implementations or public headers.

## 5. Core

The Domain layer (`lib/include/mbootcore/domain/`) provides foundation interfaces and types with zero external dependencies:

- **Interfaces**: `IProtocol`, `ITransport`, `ILogger`, `IDevice`, `ILoader`, `IFlashDevice`
- **Types**: `Result<T>` (discriminated union of `T` or `ErrorCode`), `ByteBuffer`, `DeviceInfo`, `ProtocolVersion`, `LogLevel`
- **Pattern**: All fallible operations return `Result<T>` — exceptions are not used for control flow

The Core layer adds generic utilities:

- `GenericStateMachine` — reusable state machine with event guards, transitions, reset, and cancellation support. Used by both Sahara and Firehose protocol implementations.

### Protocol Implementations

Each protocol is self-contained with its own serialization, parsing, and state management:

- **Sahara** (`sahara/`): Binary packet protocol for Qualcomm EDL mode. 19 packet types, V2/V3 support, 41 NAK codes. Own serializer (`SaharaPacketSerializer`) and parser (`SaharaPacketParser`).
- **Firehose** (`firehose/`): XML-based protocol for flash operations after programmer loading. 13 commands, streaming engine, typed command classes, XML entity handling. Does not use the binary packet model.

Protocols are isolated from each other — Sahara headers are never included by Firehose and vice versa.

### Transport Layer

- **USBTransport**: WinUSB backend for Windows, LibUSB backend for Linux/macOS
- **SerialTransport**: Native backend (POSIX termios / Win32 API)
- **TCPTransport**: Native backend (POSIX sockets / WinSock2)
- **UdpTransport**: Native backend (POSIX sockets / WinSock2)

All implement `ITransport` interface. Backend selection is centralized in `make*Backend()` factory functions.

### Generic Flash Layer

Protocol-agnostic flash abstraction:

- `IFlashDevice` — unified interface for all flash operations (open, close, read, write, erase, reset, upload)
- `FlashCapability` — bit-flag system for runtime feature detection
- `SaharaAdapter` / `FirehoseAdapter` — bridge implementations between protocol classes and `IFlashDevice`

### ELF Engine

Standalone protocol-agnostic ELF binary processor:

- `ElfParser` — 32/64-bit ELF parsing
- `ElfValidator` — validation rules engine
- `MemoryImageBuilder` — PT_LOAD segment merging sorted by virtual address
- `IProgrammerExecutor` — 5-stage execution interface (load, verify, prepare, transfer, start)
- `VirtualProgrammer` — simulated executor for testing

### GPT Engine

Complete GUID Partition Table implementation:

- `Guid` — 16-byte GUID type with string conversion
- `GPTParser` — validates protective MBR, header CRC32, entry CRC32, range validity, overlap detection
- `GPTWriter` — creates binary GPT from scratch with computed CRCs
- `PartitionManager` — full lifecycle: open, list, find, read, write, erase, backup, restore, recover
- All I/O goes through `IFlashDevice` (zero protocol dependencies)

### Discovery Framework

Registry-based device discovery and protocol negotiation:

- `IDeviceDetector` — enumerate, identify, probe physical devices
- `IProtocolNegotiator` — score descriptors against protocol capabilities
- `IProtocolFactory` — create `IFlashDevice` + `BootPipeline` from descriptors
- `ProtocolRegistry` — central registry for all detectors, negotiators, factories
- `DeviceDiscoveryEngine` — orchestration: discover all, discover by vendor, probe
- `ProtocolNegotiationEngine` — scoring-based best-match selection
- `VirtualDeviceDetector` — simulates 8+ device types without hardware

### Loader Framework

Protocol- and vendor-agnostic boot programmer manager:

- `ILoaderRepository` — file system scanning and caching
- `ILoaderMatcher` — descending priority scoring (PK Hash, MSM ID, Vendor, Chipset, Protocol, Storage)
- `ILoaderValidator` — integrity and compatibility verification
- `ElfInspector` — ELF header inspection
- `LoaderFramework` — facade orchestrating all components

### Pipeline Layer

Boot stage orchestrator:

- `BootPipeline` — coordinates all stages from Sahara handshake to Ready state
- `BootContext` — shared mutable state across stages (move-only)
- `RecoveryStrategy` — per-stage retry, rollback, and abort rules
- `BootPipelineConfig` — timeouts, retries, and flags
- Stage handlers are `std::function` — pipeline has zero protocol headers

### Session Layer

Public API for device session management:

- `DeviceSession` — wraps descriptor, flash device, pipeline, logger, state, statistics, observers
- `DeviceSessionFactory` — creates sessions from discovery descriptors via `ProtocolRegistry`
- `DeviceManager` — owns multiple sessions, find by ID/vendor/protocol/state
- `ISessionObserver` — 7 lifecycle callbacks (state, progress, error, completed, disconnect, operation started, operation finished)
- `SessionStatistics` — bytes/ops/bps with exponential moving average throughput
- `SessionLogger` — thread-safe logger with text/JSON export

### Supporting Components

| Module | Purpose |
|--------|---------|
| **Logging** (`logging/`) | ConsoleLogger, FileLogger, NullLogger, StructuredLogger (JSON-line format) |
| **Security** (`security/`) | SecurityManager, Mbed TLS backend (optional), PermissionSet, InMemoryStorage |
| **Config** (`config/`) | ConfigManager with JSON/YAML/INI parsing |
| **Telemetry** (`telemetry/`) | TelemetryCollector with counter/duration/gauge aggregation |
| **Diagnostics** (`diagnostics/`) | DiagnosticsManager, DiagnosticCheck (8 built-in health checks), DiagnosticSession, DiagnosticReport |
| **Profiler** (`profiler/`) | PerformanceProfiler — RAII scoped wall-clock + CPU profiling |
| **Memory** (`memory/`) | MemoryTracker — platform CRT hook-based allocation tracking |
| **Stress** (`stress/`) | StressTestFramework — concurrent session simulation, rapid cycles |
| **Faults** (`faults/`) | FaultInjectionEngine — configurable fault probability and filtering |
| **Benchmark** (`benchmark/`) | BenchmarkRunner — registration, execution, JSON export |

## 6. SDK

The Plugin SDK provides third-party extension points through zero-dependency public headers:

- **19 public headers** in `sdk/include/sdk/`
- **Plugin types**: Protocol, Vendor, Discovery, Negotiation, PipelineStage, RecoveryStrategy, SessionExtension, LoggingExtension, LoaderMatcher, GPTExtension
- **Lifecycle**: Load, Initialize, Enable, (run), Disable, Shutdown, Unload
- **Compatibility**: Version range checks (1-2), dependency graph with circular detection
- **Registration**: Registry-based — no hardcoded vendor or protocol names

## 7. Applications

| Application | Directory | Description |
|-------------|-----------|-------------|
| **mboot-cli** | `apps/mboot-cli/` | Command-line flashing tool with 36 commands, interactive and script modes |
| **mboot-studio** | `apps/mboot-studio/` | Qt 6 desktop GUI with 13 views, 4 themes, Runtime integration |

Qt is exclusively a dependency of `apps/mboot-studio/`. The core library (`lib/`) has zero Qt dependencies.

## 8. Plugins

MBootCore supports third-party extensions through the Plugin SDK:

- **Registration**: Registry-based with automatic discovery. No hardcoded vendor or protocol names.
- **Lifecycle**: Load, Initialize, Enable, (run), Disable, Shutdown, Unload.
- **Compatibility**: Version range checks (SDK version 1-2 accepted), dependency graph with circular detection.
- **Context**: `PluginContext` provides access to registry, engines, device manager.

New protocols implement `IProtocolPlugin`. New vendors implement `IVendorPlugin`. No `if (vendor)` branching exists outside protocol implementation directories.

## 9. Vendors

### Maturity Model

| Maturity | Meaning |
|----------|---------|
| Experimental | Enum entries only, no implementation |
| Scaffold | Reference implementation, not tested against real hardware. Gated behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`. |
| Preview | Real hardware validation locally |
| Production | Full state machines, golden vectors, hardware test suite |

### Current Coverage

| Maturity | Vendor | Protocols |
|----------|--------|-----------|
| Production | Qualcomm | Sahara, Firehose |
| Scaffold | MediaTek | BROM |
| Scaffold | UNISOC (Spreadtrum) | FDL |
| Experimental | Samsung, Apple, Huawei, Rockchip, Google | Enum entries only |

Scaffold implementations are extensibility demonstrations. They prove the architecture can accommodate new vendors without modifying platform layers.

## 10. Error Handling

- `Result<T>` in all public APIs. Errors are values, not control flow.
- `ErrorCode` is an enum covering transport, protocol, framework, and domain errors.
- No exceptions in core library code. Exception use is permitted in applications at the boundary (e.g., Qt signal handlers).
- All Mbed TLS failures are converted to `Result<T>` / `ErrorCode` at the security boundary. No exceptions cross library boundaries.

## 11. Build System

- **CMake 3.20+**, C++17, zero compiler warnings (`-Wall -Wextra -Wpedantic`)
- **Compiler support**: MinGW-w64, GCC, Clang, AppleClang

### Monolithic Library

The core library ships as a single monolithic static archive:

```txt
mbootcore_objects (OBJECT)       -- compilation: all sources, defs, includes, warnings
       |
mbootcore (STATIC)               -- assembly + export: zero link dependencies
       |
POST_BUILD monolithic merge      -- merges zlib, Mbed TLS, libusb, SDK into libmbootcore.a
       |
install(EXPORT MBootCoreTargets) -- CMake-generated targets file (only MBootCore::mbootcore)
```

Consumer integration: `find_package(MBootCore)` + `target_link_libraries(... MBootCore::mbootcore)`. Zero third-party dependencies required.

### Verification Gates

Three hermetic regression tests (`ctest -R export_validation`) prevent dependency leaks:

| Gate | Purpose |
|------|---------|
| Export Integrity | Static scan of `MBootCoreTargets.cmake` for forbidden dependency names |
| Consumer Package | Functional `find_package` + compile + link + API call in isolated temp directory |
| Install Tree Audit | Category-based file audit (no third-party artifacts in install tree) |

## 12. Testing

| Category | Description |
|----------|-------------|
| Unit tests | Single-component logic verification. No hardware, no mock frameworks for unrelated dependencies. |
| Integration tests | Component interaction verification. Tagged with `LABELS Integration` in CTest. |
| Benchmarks | Performance characteristics (latency, throughput, CPU). Not correctness tests. |
| Fuzz testing | Coverage-guided fuzz harnesses for Sahara, Firehose XML, and ELF parsers. |
| Security tests | Parser robustness and crypto regression. |

All tests use virtual devices — no physical hardware required. MemoryTracker verifies zero leaks. Fuzz testing validates 500+ random inputs across all parsers.

Test executables covering all layers:

| Category | Tests | Coverage |
|----------|-------|----------|
| Sahara & Domain | 13 | Packet types, state machines, serialization, fuzz, golden vectors, virtual |
| Firehose protocol | 5 | All 13 commands, golden vectors, fuzz, stress, fault injection, virtual |
| Generic/ELF/Pipeline | 4 | Flash, loader, ELF parsing, boot orchestration |
| GPT engine | 4 | Models, parser, writer, partition manager |
| Discovery/Session | 3 | Detection, negotiation, concurrency |
| Plugin/Job/Firmware/Workflow | 5 | Plugin lifecycle, job pipeline, firmware package, workflow, vendor framework |
| Runtime | 4 | Runtime orchestrator, dynamic library, dynamic plugin loading |
| Transport | 15 | Backend selection, USB, serial, TCP, UDP, mock, enumeration, integration, benchmark |
| CLI | 6 | Parser, formatter, commands, integration, scripting, virtual |
| Diagnostics/Security/Config | 16 | Diagnostics, profiler, memory, stress, fault, recovery, security, crypto, config, benchmark, compatibility, fuzz, longrunning, telemetry, logging, DSP |
| SDK | 16 | Vendor SDK, plugin compatibility, manifest, dependency, registration, template, version, doc generator, info, doctor, env checker, validator, API compatibility |
| Hardware (auto-skip) | 3 | Qualcomm, MediaTek, UNISOC detection — skip when no device connected |
| GUI (Studio) | 18 | All 15 widget categories + integration |
| Qt in core | 1 | Zero Qt dependency in core library |

## 13. Extension Rules

### Adding a Protocol

1. Implement `IProtocolPlugin` (see `lib/include/mbootcore/plugin/IProtocolPlugin.hpp`)
2. Implement the protocol state machine, packet types, and serializer/parser
3. Register via the plugin system — no hardcoded branches
4. Gate scaffold implementations behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`

### Adding a Vendor

Follow the maturity model:

1. **Scaffold** — enum entry + scaffold implementation, gated behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`
2. **Preview** — real hardware validation locally
3. **Production** — full state machines, golden vectors, hardware test suite

### Adding a Transport Backend

Implement the appropriate backend interface (`ISerialBackend`, `ITcpBackend`, `IUdpBackend`, `UsbBackend`) and provide a `make*Backend()` factory function. Backend selection is centralized in factory functions — platform conditionals must not leak into transports or public APIs.

### Adding a Pipeline Stage

Implement `IStageHandler` and register via the plugin system.

## 14. Non-Goals

The following are intentionally outside the scope of MBootCore:

- GUI framework (Qt remains in `apps/` only — never in `lib/`)
- IDE integration
- Firmware authoring or editing tools
- Reverse engineering utilities
- Vendor-specific hacking tools
- Package manager or OS-level flashing tool

## See Also

- [Device Discovery](DeviceDiscovery.md) — confidence scoring, registry design
- [Firmware Package Format](FirmwarePackage.md) — JSON manifest specification
- [Security](Security.md) — crypto provider architecture
- [Design Decisions](DesignDecisions.md) — rationale for key architectural choices
