# Build Guide

## Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | 3.20+ | Build system |
| C++ compiler | GCC 11+, Clang 15+ | C++17 support required |
| Qt (optional) | 6.5+ | Required only for GUI (mboot-studio) |
| MinGW-w64 | 13.x (Windows) | Official development toolchain |

## Quick Start

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -j$(nproc)
```

### CMake Presets (Recommended)

MBootCore provides CMake presets for all common configurations.
Requires **CMake 3.23+**.

```bash
# List available presets
cmake --list-presets

# Configure + build + test with a preset
cmake --preset debug -G Ninja
cmake --build --preset debug -j$(nproc)
ctest --preset default

# Sanitizer build
cmake --preset asan -G Ninja
cmake --build --preset asan
ctest --preset default
```

Available presets: `debug`, `release`, `relwithdebinfo`,
`asan`, `ubsan`, `asan-ubsan`, `tsan`, `coverage`, `fuzzing`, `clang-tidy`.
Release packaging presets (`release-package-*`) are used by the GitHub Release workflow.

See `CMakePresets.json` for the full list and `cmake/Sanitizers.cmake`
for sanitizer options.

The Qt GUI application (`mboot-studio`) builds automatically when Qt 6 is
detected. No manual flags are required. If Qt is not installed, Studio is
disabled and only the core library, CLI, SDK, and tests are built.

To override auto-detection:
```bash
cmake .. -DMBOOTCORE_BUILD_STUDIO=OFF   # force disable
cmake .. -DMBOOTCORE_BUILD_STUDIO=ON    # force enable (fails if Qt missing)
```

## Dependencies

**Qt is the only externally installed dependency.** All other third-party
libraries (nlohmann_json, zlib, MbedTLS, libusb, Catch2) are managed by the
built-in dependency manager:

- **First configure:** Archives are downloaded from GitHub, SHA256-verified,
  extracted, and statically built into `deps/<name>/`.
- **Subsequent configures:** Cached sources are reused. No network contact.
- **Cache is stable:** Deleting `build/` preserves `deps/`. Only the affected
  dependency is re-downloaded when a version changes.
- **Offline builds:** Pass `-DMBOOTCORE_OFFLINE_BUILD=ON` to fail with guidance
  if a dependency is not already cached.

See [cmake/DependencyManager.cmake](../../cmake/DependencyManager.cmake) for
the full implementation.

## Build Configuration

### CMake Options

See [BUILD_OPTIONS.md](../BUILD_OPTIONS.md) for the complete list of
CMake options including the `MBOOTCORE_WARNINGS_AS_ERRORS` policy.


### Build Targets

| Target | Type | Description |
|--------|------|-------------|
| `mbootcore` | Static library | Core framework |
| `mbootsdk` | Static library | Plugin SDK |
| `mboot-cli` | Executable | Command-line interface |
| `mboot-studio` | Executable | Desktop GUI application |

### Install Targets

```powershell
cmake --install . --prefix C:/MBootCore-install
```

The install tree includes:
- Static libraries: `libmbootcore.a`, `libmbootsdk.a`
- 200+ public headers in `include/mbootcore/`
- 19 SDK headers in `include/sdk/`
- CMake package config for `find_package(MBootCore)`
- Examples, documentation, templates, release metadata

## External Consumption

```cmake
find_package(MBootCore REQUIRED)
target_link_libraries(myapp PRIVATE MBootCore::mbootcore)
```

## Development Build Metrics

| Metric | Value |
|--------|-------|
| Full build (all test targets, 4 cores) | ~3 min 35s |
| Incremental build (no changes) | <1s |
| Library build only | ~47s |
| Test suite execution | ~27s |
| Compiler warnings | 0 (`-Wall -Wextra -Wpedantic`) |

## Supported Compilers

| Compiler | Status |
|----------|--------|
| GCC 13 / MinGW-w64 (i686) | Primary, fully tested |
| GCC 11+ (Linux x86_64) | Expected compatible |
| Clang 15+ (MinGW ABI) | Expected compatible |
| Clang 15+ (Linux, macOS) | Expected compatible |

## Supported Platforms

| Platform | Status |
|----------|--------|
| Windows 10/11 (32-bit or 64-bit, MinGW-w64) | Fully supported |
| Ubuntu 22.04+ (x86_64) | Fully supported |
| macOS 13+ (x86_64 / arm64) | Supported |

