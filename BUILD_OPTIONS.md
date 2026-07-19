# MBootCore Build Options

**Document Version:** 1.0
**Last Updated:** 2026-07-11

> This document is the reference for all supported build configurations.
> It covers CMake options, toolchain requirements, and platform-specific notes.

---

## Core Options

| Flag | Default | Description |
|------|---------|-------------|
| `MBOOTCORE_BUILD_TESTS` | `ON` | Build all test suites |
| `MBOOTCORE_BUILD_CLI` | `ON` | Build mboot-cli |
| `MBOOTCORE_BUILD_EXAMPLES` | `OFF` | Build example projects |
| `MBOOTCORE_BUILD_TOOLS` | `ON` | Build PluginWizard, DocGenerator |
| `MBOOTCORE_BUILD_STUDIO` | `ON` | Build mboot-studio (requires Qt6; option in root `CMakeLists.txt`) |
| `MBOOTCORE_ENABLE_USB` | `ON` | Enable USB transport backend. On Windows: WinUSB (built-in). On Linux/macOS: libusb auto-downloaded and statically built. Set to `OFF` to disable entirely. |
| `MBOOTCORE_OFFLINE_BUILD` | `OFF` | Fail with guidance if a dependency archive is not already cached in `deps/`. Use when no Internet access is available. |
| `MBOOTCORE_ENABLE_CRYPTO` | `ON` | Enable cryptographic backend (Mbed TLS, auto-managed; when OFF, all crypto operations return NotSupported and firmware validation uses XOR checksums only — suitable for development/testing, not production). |
| `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS` | `OFF` | Build vendor scaffold reference protocols (MediaTek, UNISOC) — not production ready |
| `BUILD_STUDIO_TESTS` | `ON` | Build mboot-studio test suite (requires Qt6 Test) |

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
