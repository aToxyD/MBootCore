# MBootCore — Project Constitution

**Document Version:** 1.4
**Last Updated:** 2026-07-17

> This document defines the permanent governance of the project.
> It changes only when architecture decisions, design rules, or conventions change.

---

## Source of Truth

The implementation is the source of truth. This document describes principles,
constraints, and conventions. It does not repeat API documentation.

---

## 1. Project Overview

Professional low-level C++17 framework for BootROM protocols.

Production support:
  Qualcomm Sahara + Firehose

Architecture scaffold implementations:
  MediaTek BROM, UNISOC FDL

Qt is permitted in applications (`apps/`, `plugins/`). The core library
(`lib/`) has no Qt dependency.

---

## 2. Technology Stack

C++17, Modern CMake.
MinGW-w64, GCC, Clang, AppleClang.

---

## 3. Repository Layout

| Directory     | Purpose                                            |
|---------------|----------------------------------------------------|
| `lib/`        | Core library: interfaces, implementations          |
| `sdk/`        | Vendor SDK: public headers and source              |
| `apps/`       | `mboot-cli/` (CLI), `mboot-studio/` (Qt 6 GUI)    |
| `tests/`      | Unit, integration, fuzz, runtime, GUI tests        |
| `examples/`   | Runnable example projects                          |
| `tools/`      | PluginWizard, DocGenerator                         |
| `docs/`       | Architecture docs, specifications, certification   |
| `scripts/`    | Release pipeline automation                        |
| `cmake/`      | CMake modules, CPack config, templates             |
| `templates/`  | Project templates (plugin scaffolding)             |
| `dist/`       | Build output staging and release archives          |

---

## 4. Architecture

### 4.1 Layers

Layered Clean Architecture with clearly defined dependency boundaries:

| Layer             | Responsibility                                              |
|-------------------|-------------------------------------------------------------|
| Domain            | Pure interfaces and types. Zero platform dependencies.     |
| Core              | Generic state machine utilities, pipeline orchestrator.    |
| Protocols         | Per-vendor protocol implementations (Sahara, Firehose).    |
| Transport         | USB, Serial, TCP, UDP backends.                             |
| Device            | Default device implementation (`IDevice`).                  |
| Loader            | LoaderFramework, LoaderManager, ProgrammerLoader.            |
| ELF               | ElfParser, ElfValidator, MemoryImageBuilder.                |
| GPT               | Guid, GPTParser, GPTWriter, PartitionManager.               |
| Discovery         | Vendor/boot-mode enums, DeviceDescriptor, negotiation.      |
| Session           | DeviceSession, DeviceManager, SessionLogger, observers.     |
| Job               | JobPipeline, JobScheduler, RecoveryPolicies.                |
| Plugin            | IPlugin, IProtocolPlugin, IVendorPlugin, PluginManager.     |
| Firmware          | FirmwarePackage, FirmwareValidator, FlashPlan.              |
| Benchmark         | BenchmarkTypes, BenchmarkRunner.                            |
| Profiler/Memory   | PerformanceProfiler, MemoryTracker (compile-time gated).    |
| Stress/Faults     | StressTestFramework, FaultInjectionEngine (compile-time gated). |
| Security          | SecurityManager, PermissionSet, InMemoryStorage, SecurityTypes. |
| Config            | ConfigManager (JSON/YAML/INI).                              |
| Telemetry         | TelemetryTypes, TelemetryCollector (compile-time gated).    |
| Logging           | ConsoleLogger, FileLogger, NullLogger, StructuredLogger.    |
| Diagnostics       | DiagnosticsManager, DiagnosticCheck, DiagnosticSession.     |
| Application       | Session (public API facade).                                |
| Runtime           | Orchestration layer coordinating services and state.         |
| GUI               | mboot-studio Qt 6 desktop application.                     |

### 4.2 Dependency Rules

```
GUI
  ↓
Runtime
  ↓
Application (Session)
  ↓
Discovery → Session → Job → Protocol → Transport → Platform → OS
```

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
- **Core library ships as a monolithic static archive** — consumers need zero
  third-party dependencies to locate. All compile properties live on
  `mbootcore_objects` (OBJECT library); the PUBLIC interface lives on
  `mbootcore` (STATIC wrapper).

### 4.3 Architectural Invariants

- Platform selection via `#ifdef` only inside factory `.cpp` files — never in
  transport implementations or public headers.
- Qt is never a PUBLIC dependency of the core library. Qt is exclusively a
  dependency of `apps/mboot-studio/` (GUI only).
- Native backends are the only implementation on all platforms.
- Factory functions are free functions, not factory classes.
- All system resources are wrapped in RAII handles.
- Connected UDP only (connect → send/recv → disconnect); no connectionless API.
- All operating-system networking APIs are confined to backend implementations.
  Transport classes are platform-agnostic and communicate exclusively through
  backend interfaces.
- Backend selection is centralized. Platform-specific backend selection is
  performed exclusively inside `make*Backend()` factory functions. Platform
  conditionals must not leak into transports or public APIs.
- **OBJECT library compilation pattern**: `mbootcore_objects` owns all compile
  properties (defs, includes, options, warnings, platform sources) and all
  PRIVATE dependencies. `mbootcore` owns the PUBLIC INTERFACE only. No
  duplication between the two targets.
- **No install() in lib/**: `lib/CMakeLists.txt` must never call
  `install()`. Installation is handled exclusively by `cmake/InstallRules.cmake`.
- **No find_dependency() in MBootCoreConfig.cmake**: The generated config file
  includes `MBootCoreTargets.cmake` directly. No third-party `find_dependency()`
  calls — all deps are merged into the monolithic archive.
- **Export-clean targets**: `mbootcore` must have zero link dependencies on
  non-exported targets. PRIVATE deps go on `mbootcore_objects`, never on
  `mbootcore`.
- **Verification gates are mandatory**: `export_validation` (3 hermetic gates)
  must pass before any build is considered complete. No exceptions.

---

## 5. Design Rules

- **Clean Architecture**: Dependencies point inward. Inner layers define interfaces;
  outer layers implement them.
- **Result\<T\>**: Use over exceptions for predictable control flow in all public APIs.
- **Explicit state machines**: Each protocol has an explicit state machine with
  transition tables.
- **No layer violations**: Core code never includes protocol headers.
- **Composition over Inheritance**: Prefer small interfaces and composed objects.
- **RAII + Move semantics**: Resources owned by RAII wrappers. Move-only types for
  shared context.
- **Strong typing**: `enum class`, `std::optional`, `std::variant`, `std::string_view`.
  No raw integers for semantic types.
- **const correctness**: Mark methods and parameters `const` by default.
  Immutable after construction where possible.
- **ISP compliance**: `IProtocol` does not depend on `ILogger`. `IDevice` does not
  expose `transport()`.
- **Session API**: Accepts dependencies via setter methods rather than forced
  constructor injection.
- **LoaderMetadata**: Struct with raw key-value map. Matching uses descending
  priority scoring.
- **Firehose**: Uses XML messages, not `IPacket`.
- **Plugins**: Registration is automatic via registry. Compatibility version
  check accepts 1–2.
- **I-prefix for interfaces**: `ISerialBackend`, `ITcpBackend`, `IUdpBackend`.
  (Exception: `UsbBackend` — retained for historical compatibility.)
- **Free functions for factories**: `makeUsbBackend()`, `makeSerialBackend()`,
  `makeTcpBackend()`, `makeUdpBackend()`. No factory classes, no enums passed
  to factory functions, no `availableBackends()`. (Domain enums like
  `TransportType` exist separately and are used by transport implementations.)
- **NetworkAddress**: Value object in `mbootcore::network` namespace. Immutable,
  IPv4+IPv6+hostname, `==`/`!=`/`<`/`Hash`, `loopback()`, `any()`.
- **Semantic–Serialization Separation**: Semantic objects (`Command`, `Request`,
  `Response`, `Event`, `CapabilityDescriptor`) must never depend on serialization
  objects (`ByteBuffer`, `PacketEncoder`, `PacketDecoder`, wire formats). Protocol
  semantics and transport mechanics are independent concerns.
- **Frozen Layers**: The Transport Platform, Runtime Orchestration, and Protocol
  Platform (Vocabulary, Semantics, Serialization) are architecturally validated
  and frozen. Modifications require strong engineering justification and
  maintainer approval.
- **Frozen Build System**: The monolithic library architecture (OBJECT + STATIC
  wrapper + monolithic archive merge + install(EXPORT) + 3 verification gates)
  is architecturally validated and frozen. Modifications require strong
  engineering justification and maintainer approval.
- **ITransport capability advertising**: Each transport implementation must
  advertise its capabilities via `capabilities()` returning `TransportCapability`
  flags. No implicit assumptions about transport behavior.
- **ITransport pure virtual contract**: `transportType()`, `config()`,
  `setConfig()`, and `capabilities()` are pure virtual on `ITransport`. Methods
  with valid default semantics (`state()`, `statistics()`, `flush()`, `reset()`,
  `reconnect()`, `progress()`, `endpoint()`) retain default implementations.
  `TransportFactory` is actively maintained (not deprecated); it provides
  pre-configured `ITransport` wrappers for common scenarios.
- **ProtocolVersion consolidation**: A single `mbootcore::ProtocolVersion`
  `{major, minor}` value-type resides in the domain layer. Vendor-specific
  version semantics use prefixed types (e.g., `SaharaNegotiatedVersion`).
- **No legacy aliases**: Deprecated `using` aliases (`SerialBackend`,
  `TcpBackend`) are removed. No compatibility shims for migrated interfaces.

---

## 6. Coding Standards

### 6.1 Naming

| Category            | Convention                          | Example                    |
|---------------------|-------------------------------------|----------------------------|
| Classes             | PascalCase                          | `UsbBackend`              |
| Interfaces          | `I` + PascalCase                    | `ISerialBackend`          |
| Functions           | camelCase                           | `makeUsbBackend()`        |
| Variables           | camelCase                           | `portName`                |
| Instance Members    | `m_` + camelCase                    | `m_transport`             |
| Named Constants     | `k` + PascalCase                    | `kMaxRetryCount`          |
| Constants           | PascalCase                          | `MaxRetryCount`           |
| Macros              | `UPPER_SNAKE_CASE`                  | `MBOOTCORE_HAVE_LIBUSB`   |
| Files               | PascalCase with `.hpp`/`.cpp`       | `NetworkAddress.hpp`      |
| Namespaces          | lowercase, dot-separated            | `mbootcore::transport`    |

### 6.2 Memory Management

- No raw owning pointers. Use `std::unique_ptr` for exclusive ownership,
  `std::shared_ptr` for shared ownership.
- RAII for all system resources (handles, sockets, file descriptors).
- Move semantics for shared context objects (`BootPipelineContext`).
- No global state, no singletons, no god classes.
- Never write into a `std::vector` after `reserve()` alone. Writable storage
  must be established with `resize()` (or an equivalent owning container).
  After `reserve()`, `data()` returns a formally invalid range when `size()`
  is zero, inviting UB from compiler optimizations.

### 6.3 Error Handling

- `Result<T>` in all public APIs. Errors are values, not control flow.
- No exceptions in core library code.
- Exception use is permitted in applications at the boundary (e.g., Qt signal
  handlers).

### 6.4 Thread Safety

- `const` methods are thread-safe by convention.
- Use `mutable std::mutex` for internal caching when needed.
- Document thread-safety guarantees in interface headers.

### 6.5 Platform Abstraction

- `#ifdef` guards only in `.cpp` files that select implementations.
- Never in public headers or transport implementation classes.
- Platform-specific backends live in their own files, selected at build time
  via CMake platform blocks.

### 6.6 Performance

- Zero-cost abstractions where possible.
- No virtual dispatch in hot data paths.
- Move semantics to avoid copies.
- No heap allocation in transport read/write hot paths.

### 6.7 Documentation

- Doxygen for all public API headers.
- Inline comments for non-obvious logic or workarounds.
- No comments that restate what the code says.

---

## 7. Thread Safety

### 7.1 Lock Hierarchy

All synchronization follows a strict lock hierarchy to prevent deadlocks:

```
Operation Mutex (Runtime::m_opMutex)
          ↓
Statistics Mutex (RuntimeState::m_statsMutex)
          ↓
ObserverManager Mutex (ObserverManager::m_mutex)
```

- Locks must always be acquired in this order, never reversed.
- Acquiring the ObserverManager mutex while holding opMutex is permitted.
- Acquiring opMutex while holding ObserverManager mutex is forbidden.
- PluginManager mutex is acquired independently, never nested inside any
  Runtime mutex.

### 7.2 Callback Rules

Callbacks (observers, progress, health, statistics) must never be invoked
while holding a lock that protects the callback container. The pattern for
all notification methods:

```
Copy observers/listeners under lock
Release lock
Iterate over the copy
```

This prevents:
- Callback-under-lock deadlocks (observer calls back into Runtime API)
- Iterator invalidation (observer modifies the container during callback)
- Re-entrancy deadlocks (nested notify → acquire same mutex)

### 7.3 Observer Guarantees

- `addObserver`/`removeObserver` are safe to call from any thread.
- `addObserver`/`removeObserver` are safe to call from within a callback
  for the same observer type.
- Observer callbacks use a copy of the observer list; observers added
  during a callback will be notified on the next event, not the current one.
- Observers removed during a callback will not receive further callbacks
  after the current iteration.
- `RuntimeObserver` callbacks hold a `std::shared_ptr` with a null
  deleter for the duration of the notification. The observer object must
  outlive the `ObserverManager`.

### 7.4 Re-Entrancy Policy

Re-entrant observer callbacks are supported. An observer may call
`notifyEvent()` from within `onRuntimeEvent()`. Each re-entrant call
produces a fresh copy of the observer list, preventing iterator
invalidation.

Recursive re-entrancy is bounded by available stack space. Observers
SHOULD NOT rely on unbounded re-entrancy in production code.

### 7.5 Thread Ownership

| Component       | Thread Model                                            |
|-----------------|---------------------------------------------------------|
| Runtime         | Thread-safe via m_opMutex + m_statsMutex                |
| ObserverManager | Thread-safe via m_mutex + copy-under-lock pattern       |
| DeviceSession   | Thread-safe via atomics + m_observersMutex               |
| DeviceManager   | Thread-safe via shared_mutex (readers-writer)           |
| PluginManager   | Thread-safe via m_mutex (locked helpers)                |
| JobScheduler    | Single worker thread, thread-safe queue                 |
| SessionLogger   | Thread-safe via m_mutex (fast in-memory ops)            |
| VendorRegistry  | Thread-safe via shared_mutex (readers-writer)           |
| VendorMonitor   | Thread-safe via m_mutex                                  |

### 7.6 Atomic Usage

- `SessionState` and cancellation flags use `std::atomic` with default
  `memory_order_seq_cst` for correctness.
- `PluginManager` counters use relaxed ordering where visible side effects
  are not required.
- All atomics protect single variables only; compound state changes are
  guarded by mutexes.

### 7.7 Condition Variables

All `std::condition_variable` waits must use a predicate to guard against
spurious wakeups:

```cpp
cv.wait(lock, predicate);   // CORRECT
cv.wait(lock);              // FORBIDDEN
```

The predicate must check the condition and the lifetime flag.

### 7.8 Shutdown Safety

- `shutdown()` moves state mutation under a minimal critical section,
  then releases the lock before invoking plugin shutdown, service
  destruction, or event notifications.
- No `detach()` on threads. All threads are `join()`-ed in the
  destructor or during shutdown.
- `JobScheduler` drains its queue under lock before stopping.

---

## 8. Public Components

| Component      | Header Path                              | Purpose                           |
|----------------|------------------------------------------|-----------------------------------|
| Session        | `mbootcore/application/Session.hpp`      | Main public API facade            |
| Runtime        | `mbootcore/runtime/Runtime.hpp`          | Application service orchestrator  |
| Plugin SDK     | `sdk/`                                   | Vendor plugin interfaces          |
| Transport      | `mbootcore/transport/`                   | ISerialBackend, ITcpBackend, ...  |
| Discovery      | `mbootcore/discovery/`                   | Device detection, negotiation     |
| Pipeline       | `mbootcore/pipeline/`                    | Boot orchestrator                 |
| Firmware       | `mbootcore/firmware/`                    | Package loading, validation       |
| GPT            | `mbootcore/gpt/`                         | Partition table management        |
| Diagnostics    | `mbootcore/diagnostics/`                 | Health checks, system report      |
| CLI            | `apps/mboot-cli/`                        | Command-line flashing tool        |
| Studio         | `apps/mboot-studio/`                     | Qt 6 desktop GUI                  |

---

## 9. Build

### CMake Presets (Recommended)

Requires CMake 3.20+ (3.23+ for presets).  All build scripts and the release workflow use presets internally.

```bash
cmake --list-presets                          # list available presets
cmake --preset debug -G Ninja                # configure
cmake --build --preset debug -j$(nproc)      # build
ctest --preset default                       # test
```

### Windows (MinGW-w64)

```powershell
$env:Path = "C:\winlibs\mingw32\bin;$env:Path"
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1-static/mingw_32"
cmake --build . -j4
ctest --output-on-failure -j4
```

### Linux (GCC / Clang)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -j$(nproc)
```

### macOS (AppleClang)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(sysctl -n hw.logicalcpu)
ctest --output-on-failure -j$(sysctl -n hw.logicalcpu)
```

---

## 10. Testing Philosophy

- **Unit tests** verify single-component logic. No hardware, no mock frameworks
  for unrelated dependencies.
- **Integration tests** verify component interaction. Tagged with
  `LABELS Integration` in CTest.
- **Benchmarks** measure performance characteristics (latency, throughput, CPU).
  Benchmarks are not correctness tests.
- **Code coverage must never regress.** New code requires matching new tests.

---

## 11. Definition of Done

A change is done when all of the following are satisfied:

1. `cmake --build` succeeds with zero warnings.
2. `ctest` passes (new and existing tests).
3. New tests cover the change (unit, integration, or both).
4. No new static-analysis warnings (clang-tidy or configured equivalent).
5. Public API changes are documented (Doxygen).
6. `CMakeLists.txt` is updated if files or dependencies changed.
7. `AGENTS.md` is updated only if architecture decisions or design rules changed.
8. `export_validation` test passes (3 hermetic gates: Export Integrity,
   Consumer Package, Install Tree Audit).

---

## 12. Review Checklist

Every merge request must satisfy all of the following:

| #  | Criterion                        | Description                                            |
|----|----------------------------------|--------------------------------------------------------|
| 1  | Clean Architecture               | No layer violation; inner layers never depend on outer |
| 2  | SOLID                            | Single responsibility; open/closed; LSP; ISP; DIP     |
| 3  | RAII                             | All resources wrapped; no manual new/delete            |
| 4  | No platform leaks                | No `#ifdef` in public headers or transport impls      |
| 5  | No raw owning pointers           | `unique_ptr` / `shared_ptr` / references              |
| 6  | Thread safety                    | Const = thread-safe; mutex where needed               |
| 7  | Backward compatibility           | Existing API unchanged unless intentionally breaking  |
| 8  | Error handling                   | `Result<T>` for public APIs; no exceptions in core    |
| 9  | Const correctness                | Methods const by default                              |
| 10 | Move semantics                   | Move constructors/assignment where applicable         |
| 11 | Naming conventions               | Matches §6.1                                           |
| 12 | Test coverage                    | New code has matching tests; no existing test broken  |
| 13 | Documentation                    | Doxygen for public API; inline for non-obvious logic  |
| 14 | No dead code                     | No commented-out code, no unused variables/functions  |
| 15 | Monolithic integrity             | No dep leak in export file; no third-party artifacts in install tree |

---

## 13. Development Workflow

- Each change MUST merge independently — `cmake --build` and `ctest` pass
  in isolation.

---

## 14. Development Rules

- Every new public header must be exported through `MBootCore.hpp`.
- Every new source file must be added to a `CMakeLists.txt`.
- Keep layer boundaries intact. No protocol-specific code outside protocol modules.
- Maintain backward compatibility unless intentionally breaking APIs.
- Feature-gated components (profiler, memory tracker, stress, faults, telemetry)
  must compile cleanly when disabled.
- GUI code must never include protocol headers — communicate only through Runtime.
- When adding a new vendor or protocol, implement `IProtocolPlugin` and register
  through the plugin system. No hardcoded branches.

---

## 15. Agent Expectations

- Read the implementation before making architectural decisions.
- Preserve backward compatibility whenever practical.
- Prefer minimal, localized changes over large refactors.
- Do not introduce new abstractions without clear benefit.
- Apply the Definition of Done (§11) and Review Checklist (§12) to every change.
- Keep documentation synchronized with implementation.
- Treat the implementation as the source of truth.

---

## Glossary

| Term             | Meaning                                                    |
|------------------|------------------------------------------------------------|
| Core             | The `lib/` directory and its contents                      |
| Native Backend   | Platform API implementation (WinUSB, Win32, Posix, etc.)   |
| Transport        | USB / Serial / TCP / UDP abstraction layer                 |
| Public API       | Headers under `lib/include/mbootcore/`                     |
| Domain           | Pure interfaces and types with zero platform dependencies  |
| NetworkAddress   | Value type in `mbootcore::network` for network endpoints   |
| Result\<T\>      | Error-or-value return type used in all public APIs         |
| OBJECT library   | CMake target that compiles sources into `.o` files without producing an archive; owns all compile properties and PRIVATE deps |
| Monolithic archive | `libmbootcore.a` — single static archive containing MBootCore + all dependency objects (zlib, Mbed TLS, libusb, SDK) |
| Export validation | Three hermetic regression gates (Export Integrity, Consumer Package, Install Tree Audit) preventing dependency leaks |
