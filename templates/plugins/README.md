# MBootCore Plugin Templates

This directory contains ready-to-compile C++17 templates for creating
MBootCore plugins. Each template uses `%VARIABLE_NAME%` placeholders
that must be replaced with actual values before building.

## Available Templates

| Template | Description | Interface |
|---|---|---|
| `vendor/main.cpp.in` | Vendor plugin — registers VID/PID, chipsets, boot modes | `IVendorPlugin` |
| `protocol/main.cpp.in` | Protocol plugin — implements a new protocol type | `IProtocolPlugin` |
| `workflow/main.cpp.in` | Workflow plugin — multi-step device workflow | `IWorkflow` |
| `job/main.cpp.in` | Job plugin — reusable device operation | `IJob` |
| `transport/main.cpp.in` | Transport plugin — custom I/O backend | `ITransport` |

## Usage

1. **Pick a template** matching your plugin type.

2. **Copy the template** to your project:
   ```
   cp templates/plugins/vendor/main.cpp.in myplugin/main.cpp
   ```

3. **Replace placeholders** with actual values:

   | Placeholder | Example | Applies To |
   |---|---|---|
   | `%PLUGIN_NAME%` | `MyVendorPlugin` | CMakeLists.txt |
   | `%VENDOR_NAME%` | `Qualcomm` | vendor |
   | `%VENDOR_ENUM%` | `Qualcomm` | vendor |
   | `%PROTOCOL_NAME%` | `MyProtocol` | protocol |
   | `%PROTOCOL_TYPE%` | `Sahara` | protocol |
   | `%WORKFLOW_NAME%` | `FlashFlow` | workflow |
   | `%JOB_NAME%` | `SecureErase` | job |
   | `%JOB_ID%` | `secure-erase-v1` | job |
   | `%JOB_OPERATION%` | `Erasing` | job |
   | `%TRANSPORT_NAME%` | `Bluetooth` | transport |
   | `%TRANSPORT_ENUM%` | `Bluetooth` | transport |
   | `%AUTHOR%` | `Your Name` | all |
   | `%LICENSE%` | `MIT` | all |
   | `%UUID%` | `a1b2c3d4-...` | all |
   | `%VID%` / `%PID%` | `05C6` / `9008` | vendor |
   | `%CHIPSET_IDS%` | `0x1234, 0x5678` | vendor |
   | `%BOOT_MODE%` | `EDL` | vendor |
   | `%STEP_N_NAME%` | `DetectDevice` | workflow |
   | `%TOTAL_BYTES%` | `1048576` | job |
   | `%CHUNK_SIZE%` | `65536` | job |

4. **Build** (see below).

5. **Install** the built library to your application's plugin directory.

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DMBootCore_DIR="path/to/MBootCore/build/cmake"
cmake --build . -j4
```

## Usage

Plugins are loaded statically via `PluginManager::load(unique_ptr<IPlugin>, PluginConfig)`.
See `docs/guides/Plugin-Development.md` for the loading and lifecycle API.

```bash
cmake --install . --prefix /path/to/your/app
```

## API Compatibility

All templates target `compatibilityVersion = 1`. The MBootCore PluginManager
accepts plugins with compatibility versions 1 through 2.
