# MBootCore

C++17 framework for BootROM protocols.

**Production support:**
- Qualcomm Sahara (binary)
- Qualcomm Firehose (XML)

**Reference scaffolds** (extensibility demos — not production ready):
- MediaTek BROM
- UNISOC FDL

Platforms: Windows, Linux, macOS.
Architecture: Clean Architecture with strict dependency layering. Core library compiles with zero warnings.

**Security:** Cryptographic operations use the MbedTLS backend by default
(`MBOOTCORE_ENABLE_CRYPTO=ON`). Set to `OFF` to disable crypto; all
operations return `NotSupported`.
See [SecurityPolicy.md](docs/security/SecurityPolicy.md) and [docs/architecture/Security.md](docs/architecture/Security.md).

## Quick Start

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -j$(nproc)
```

Qt is not required for the core library, CLI, or SDK. The Qt GUI application
(`mboot-studio`) builds automatically when Qt 6 is detected. If Qt is not
installed, Studio is silently disabled and the rest of the framework builds
normally.

## Build Options

| Flag | Default | Description |
|------|---------|-------------|
| `MBOOTCORE_ENABLE_USB` | `ON` | USB transport backend (WinUSB on Windows; libusb on Linux/macOS) |
| `MBOOTCORE_ENABLE_CRYPTO` | `ON` | Mbed TLS cryptographic backend (auto-managed)
| `MBOOTCORE_BUILD_TESTS` | `ON` | Build all test suites |
| `MBOOTCORE_BUILD_CLI` | `ON` | Build mboot-cli |
| `MBOOTCORE_BUILD_STUDIO` | `ON` | Build mboot-studio (requires Qt6) |

### USB Backend

USB support follows the project's dependency model:
- **Windows:** WinUSB (built-in, no external dependency)
- **Linux/macOS:** libusb is auto-downloaded and statically built (no system
  package required)
- For offline builds, see `OFFLINE_BUILD` in [BuildOptions.md](docs/build/BuildOptions.md)

Disable USB entirely with `-DMBOOTCORE_ENABLE_USB=OFF`.

Configure output shows the active USB backend:

```
USB Backend
  Status:  ENABLED
  Provider: System libusb (pkg-config)
  Version:  1.0.28
```

## Documentation

Start at **[docs/README.md](docs/README.md)** — covers architecture, protocols, specifications, build, configuration, testing, plugin development, and the MBoot Studio GUI.

### Vendor Documentation

- **[Vendor Maturity Model](docs/vendor/VENDOR_MATURITY.md)** — Formal maturity states with measurable exit criteria
- **[Graduation Checklists](docs/vendor/VENDOR_GRADUATION.md)** — Per-vendor checklist from current state to Production
- **[Hardware Validation](docs/vendor/HARDWARE_VALIDATION.md)** — Hardware test framework and process
- **[Golden Vector Policy](docs/vendor/GOLDEN_VECTOR_POLICY.md)** — Vector creation, maintenance, and validation rules
- **[Scaffold Philosophy](docs/vendor/SCAFFOLD_PHILOSOPHY.md)** — Why MediaTek and UNISOC remain Scaffold
