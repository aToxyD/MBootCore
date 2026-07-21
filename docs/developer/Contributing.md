# Contributing to MBootCore

> Before opening a pull request, please read [AGENTS.md](../internal/AGENTS.md).
> It is the project constitution and defines all architectural rules,
> coding standards, and review criteria.

## Project Philosophy

MBootCore is a professional C++17 framework for BootROM protocols, built on
Clean Architecture with strict layer separation. Key principles:

- **Result\<T\>** over exceptions in all core library APIs
- **Qt-free core** — Qt is only a dependency of `apps/mboot-studio/`
- **Composition over inheritance** — small interfaces, composed objects
- **RAII and move semantics** — no raw owning pointers
- **No global state** — explicit dependency injection

See [AGENTS.md §1](../internal/AGENTS.md#1-project-overview) and
[§5 Design Rules](../internal/AGENTS.md#5-design-rules) for the full design philosophy.

## Architecture Stability

MBootCore follows established architectural invariants. Pull requests must
not redesign the architecture. Improvements should be incremental unless a
formal architectural review is completed with strong engineering justification
and maintainer approval.

Key frozen areas include the transport layer API, Qt-free core library,
runtime orchestration layering, protocol platform, and monolithic library
distribution. See [AGENTS.md](../internal/AGENTS.md#4-architecture) for the full list.

## Getting Started

### Prerequisites

- CMake 3.23+ (presets require 3.23)
- C++17 compiler: GCC 11+, Clang 15+, AppleClang, or MSVC
- Ninja (recommended) or platform-native build tool

### Configure

```bash
cmake --list-presets                  # see available presets
cmake --preset debug -G Ninja         # configure for development
```

### Build

```bash
cmake --build --preset debug -j$(nproc)
```

### Test

```bash
ctest --preset default --output-on-failure
```

See [docs/build/Build.md](../build/Build.md) for full build instructions
including platform-specific notes.

## CMake Presets

| Preset | Purpose |
|--------|---------|
| `debug` | Development with tests, security tests, warnings-as-errors |
| `release` | Minimal release (no tests, CLI, Studio, examples) |
| `release-package*` | Release packaging presets (hidden base + platform variants) |
| `asan`, `ubsan`, `tsan` | Sanitizer builds |
| `coverage` | Code coverage with gcovr |
| `fuzzing` | Fuzz testing with libFuzzer |
| `clang-tidy` | Static analysis |

See `CMakePresets.json` for the complete list.

## Code Style

- **Formatting:** Google-based style via `.clang-format`
  - IndentWidth: 4, ColumnLimit: 100, PointerAlignment: Left
- **Static analysis:** `.clang-tidy` with bugprone, performance, modernize,
  readability, cppcoreguidelines, and clang-analyzer checks
  - `bugprone-*` and `clang-analyzer-*` are warnings-as-errors

### Naming Conventions

| Category | Convention | Example |
|----------|-----------|---------|
| Classes | PascalCase | `UsbBackend` |
| Interfaces | `I` + PascalCase | `ISerialBackend` |
| Functions | camelCase | `makeUsbBackend()` |
| Variables | camelCase | `portName` |
| Instance Members | `m_` + camelCase | `m_transport` |
| Named Constants | `k` + PascalCase | `kMaxRetryCount` |
| Macros | `UPPER_SNAKE_CASE` | `MBOOTCORE_HAVE_LIBUSB` |
| Files | PascalCase `.hpp`/`.cpp` | `NetworkAddress.hpp` |
| Namespaces | lowercase, dot-separated | `mbootcore::transport` |

See [AGENTS.md §6](../internal/AGENTS.md#6-coding-standards) for complete coding standards.

## Running Formatting Checks

```bash
# Check formatting (dry-run)
clang-format-15 --dry-run --Werror \
    lib/ tests/ sdk/ apps/ tools/ examples/

# Auto-format
clang-format-15 -i <files>
```

Run this check locally before committing.

## Running Static Analysis

```bash
cmake --preset clang-tidy -G Ninja
cmake --build --preset clang-tidy -j$(nproc)
```

Run this analysis locally before committing.

## Running Tests

```bash
ctest --preset default --output-on-failure          # all tests
ctest --preset default -L "Security"                 # security tests only
ctest --preset default -R "cli_formatter_test"       # specific test
ctest --preset asan --output-on-failure              # under AddressSanitizer
```

### Test Labels

| Label | Description |
|-------|-------------|
| `Environment` | Runtime-dependent tests (may fail in containers) |
| `Integration` | Component interaction tests |
| `Security` | Parser robustness and crypto tests |
| `Benchmark` | Performance benchmarks (not correctness) |

See [docs/testing/Testing.md](../testing/Testing.md) for testing philosophy
and conventions.

## Adding a Protocol

1. Implement `IProtocolPlugin` (see `lib/include/mbootcore/plugin/IProtocolPlugin.hpp`)
2. Implement the protocol state machine, packet types, and serializer/parser
3. Register via the plugin system — no hardcoded branches
4. Gate scaffold implementations behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`

See [docs/sdk/PluginDevelopment.md](../sdk/PluginDevelopment.md)
for the full guide with code examples.

## Adding a Vendor

Follow the documented maturity model:

1. **Scaffold** — enum entry + scaffold implementation, gated behind
   `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`
2. **Preview** — real hardware validation locally
3. **Production** — full state machines, golden vectors, hardware test suite

See [docs/vendor/MaturityModel.md](../vendor/MaturityModel.md) for
measurable exit criteria and
[docs/vendor/Graduation.md](../vendor/Graduation.md) for
per-vendor graduation checklists.

## Extension Policy

The platform is **open for extension** and **closed for modification**.
New capabilities should be added by implementing existing extension points:

- New protocol → `IProtocolPlugin`
- New transport backend → `ISerialBackend` / `ITcpBackend` / `IUdpBackend`
- New vendor → `IVendorPlugin`
- New pipeline stage → `IStageHandler`
- New recovery strategy → `IRecoveryStrategy`

No hardcoded `if (vendor)` branches. Use the plugin system.

See [AGENTS.md](../internal/AGENTS.md#14-development-rules) for the extension policy.

## Documentation Expectations

- **Public API headers:** Doxygen comments for all public classes, methods,
  and free functions
- **Non-obvious logic:** Inline comments explaining *why*, not *what*
- **No restating comments:** Do not write comments that restate what the
  code says

See [AGENTS.md §6.7](../internal/AGENTS.md#66-documentation) for documentation standards.

## Pull Request Expectations

Each pull request should:

1. **Merge independently** — `cmake --build` and `ctest` pass in isolation
2. **Have a clear scope** — one logical change per PR
3. **Include matching tests** — new code requires new tests
4. **Pass all local checks** — build, tests, formatting, static analysis
5. **Not break backward compatibility** — unless intentionally breaking APIs
   (requires major version bump)
6. **Keep layer boundaries intact** — no protocol code outside protocol
   modules, no Qt in core library

### Definition of Done

A change is done when all of the following are satisfied:

1. `cmake --build` succeeds with zero warnings
2. `ctest` passes (new and existing tests)
3. New tests cover the change
4. No new static-analysis warnings
5. Public API changes are documented (Doxygen)
6. `CMakeLists.txt` is updated if files or dependencies changed

See [AGENTS.md §11](../internal/AGENTS.md#11-definition-of-done) for the full checklist.

### Review Checklist

Every merge request must satisfy:

1. Clean Architecture — no layer violations
2. SOLID — single responsibility, open/closed, LSP, ISP, DIP
3. RAII — all resources wrapped, no manual new/delete
4. No platform leaks — no `#ifdef` in public headers
5. No raw owning pointers — `unique_ptr` / `shared_ptr` only
6. Thread safety — const = thread-safe, mutex where needed
7. Error handling — `Result<T>` for public APIs
8. Const correctness — methods const by default
9. Move semantics — where applicable
10. Naming conventions — matches §6.1
11. Test coverage — new code has matching tests
12. Documentation — Doxygen for public API

See [AGENTS.md §12](../internal/AGENTS.md#12-review-checklist) for the complete review
checklist.

---

Thank you for contributing to MBootCore.

## See Also

- [Thread Safety](ThreadSafety.md) — threading model and synchronization
- [Coding Standards](CodingStandards.md) — code style rules (pointer to AGENTS.md §6)
