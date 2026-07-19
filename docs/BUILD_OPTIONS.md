# Build Options

All CMake options for MBootCore. Pass these with `-D` at configure time.

```bash
cmake .. -DOPTION=VALUE
```

## Core Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CMAKE_BUILD_TYPE` | STRING | `Release` | Build type (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`) |
| `CMAKE_INSTALL_PREFIX` | PATH | platform-dependent | Install prefix |
| `MBOOTCORE_BUILD_TESTS` | BOOL | `ON` | Build unit and integration tests |
| `MBOOTCORE_BUILD_CLI` | BOOL | `ON` | Build `mboot-cli` command-line tool |
| `MBOOTCORE_BUILD_STUDIO` | BOOL | `ON` | Build `mboot-studio` Qt GUI (auto-disabled if Qt6 missing) |
| `MBOOTCORE_BUILD_EXAMPLES` | BOOL | `OFF` | Build example projects |
| `MBOOTCORE_BUILD_TOOLS` | BOOL | `ON` | Build developer tools (PluginWizard, DocGenerator) |
| `MBOOTCORE_OFFLINE_BUILD` | BOOL | `OFF` | Fail if cached dependencies are missing |

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
MBootCore project targets. Qt, MbedTLS, zlib, libusb, nlohmann_json, and
Catch2 are compiled without `-Werror`.

**Compiler flags applied:**

| Compiler | Warning flags | Errors flag |
|----------|---------------|-------------|
| GCC / Clang | `-Wall -Wextra -Wpedantic` | `-Werror` |
| MSVC | `/W4 /permissive-` | `/WX` |

## Feature Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `MBOOTCORE_ENABLE_CRYPTO` | BOOL | auto | Enable MbedTLS crypto provider |
| `MBOOTCORE_ENABLE_USB` | BOOL | auto | Enable USB transport backend |
| `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS` | BOOL | `OFF` | Build MediaTek / UNISOC scaffold implementations |
