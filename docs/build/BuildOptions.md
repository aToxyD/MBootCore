# MBootCore Build Options

**Document Version:** 1.0
**Last Updated:** 2026-07-21

> This document is the reference for all supported build configurations.
> It covers CMake options, toolchain requirements, and platform-specific notes.

All CMake options for MBootCore. Pass these with `-D` at configure time.

```bash
cmake .. -DOPTION=VALUE
```

---

## Core Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CMAKE_BUILD_TYPE` | STRING | `Release` | Build type (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`) |
| `CMAKE_INSTALL_PREFIX` | PATH | platform-dependent | Install prefix |
| `MBOOTCORE_BUILD_TESTS` | BOOL | `ON` | Build unit and integration tests |
| `MBOOTCORE_BUILD_CLI` | BOOL | `ON` | Build `mboot-cli` command-line tool |
| `MBOOTCORE_BUILD_STUDIO` | BOOL | `ON` | Build `mboot-studio` Qt GUI (auto-disabled if Qt 6 missing) |
| `MBOOTCORE_BUILD_EXAMPLES` | BOOL | `OFF` | Build example projects |
| `MBOOTCORE_BUILD_TOOLS` | BOOL | `ON` | Build developer tools (PluginWizard, DocGenerator) |
| `MBOOTCORE_BUILD_STUDIO_TESTS` | BOOL | `ON` | Build mboot-studio test suite (requires Qt 6 Test) |
| `MBOOTCORE_OFFLINE_BUILD` | BOOL | `OFF` | Fail if cached dependencies are missing |

---

## Quality Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `MBOOTCORE_WARNINGS_AS_ERRORS` | BOOL | `ON` for Debug/CI, `OFF` otherwise | Treat compiler warnings as errors |

### Warnings-as-Errors

MBootCore enforces a zero-warning policy. The `MBOOTCORE_WARNINGS_AS_ERRORS`
option adds `-Werror` (GCC/Clang) or `/WX` (MSVC) to all MBootCore targets.

**Default behavior:**

- **Debug builds:** ON
- **CI builds** (detected via `CI` environment variable): ON
- **Release / packaging builds:** OFF
- **Downstream consumers:** OFF unless explicitly enabled

**How to disable locally:**

```bash
cmake .. -DMBOOTCORE_WARNINGS_AS_ERRORS=OFF
```

**Third-party dependencies are never affected** — the policy applies only to
MBootCore project targets. Qt, Mbed TLS, zlib, libusb, nlohmann_json, and
Catch2 are compiled without `-Werror`.

**Compiler flags applied:**

| Compiler | Warning flags | Errors flag |
|----------|---------------|-------------|
| GCC / Clang | `-Wall -Wextra -Wpedantic` | `-Werror` |
| MSVC | `/W4 /permissive-` | `/WX` |

---

## Feature Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `MBOOTCORE_ENABLE_CRYPTO` | BOOL | auto | Enable MbedTLS crypto provider |
| `MBOOTCORE_ENABLE_USB` | BOOL | auto | Enable USB transport backend |
| `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS` | BOOL | `OFF` | Build MediaTek / UNISOC scaffold implementations |

---

## Backend Selection

Backend selection is automatic based on the target platform:

| Platform | Serial Backend | TCP Backend | UDP Backend | USB Backend |
|----------|---------------|-------------|-------------|-------------|
| Windows  | Win32Serial    | WinSockTcp  | WinSockUdp  | WinUSB (built-in) |
| Linux    | PosixTermios   | PosixTcpSocket | PosixUdpSocket | libusb (auto-downloaded, static) |
| macOS    | PosixTermios   | PosixTcpSocket | PosixUdpSocket | libusb (auto-downloaded, static) |

USB backend is optional. Disable with `-DMBOOTCORE_ENABLE_USB=OFF`.

---

## Platform Matrix

| Platform | Backend | Status |
|----------|---------|--------|
| Windows  | Native  | Active |
| Linux    | Native  | Active |
| macOS    | Native  | Active (POSIX backends) |

---

## Toolchain Notes

### Windows (MinGW-w64)

- Tested with winlibs GCC 13+.

```powershell
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
ctest --output-on-failure -j4
```

### Windows (MSVC)

- Tested with Visual Studio 2022.
- Use `-G "Visual Studio 17 2022"` or Ninja.

### Linux (GCC / Clang)

- Debian/Ubuntu packages: `build-essential`, `cmake`, `libudev-dev`,
  `libusb-1.0-0-dev`.

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -j$(nproc)
```

### macOS (AppleClang)

- Xcode Command Line Tools.
- Homebrew: `libusb`.

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(sysctl -n hw.logicalcpu)
ctest --output-on-failure -j$(sysctl -n hw.logicalcpu)
```
