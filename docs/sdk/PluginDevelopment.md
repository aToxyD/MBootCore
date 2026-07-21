# Plugin Development Guide

## Overview

This guide covers creating third-party plugins for MBootCore using the Plugin SDK. Plugins can extend the framework with new protocols, vendors, transports, workflows, jobs, package formats, or discovery modules — without modifying MBootCore itself.

## Getting Started

### Prerequisites

- MBootCore SDK installed (via `cmake --install` or `find_package(MBootCore)`)
- C++17 compiler
- CMake 3.20+

### Using Plugin Templates

MBootCore provides template generators that create skeleton plugin projects:

```powershell
# List available templates
mboot-plugin-wizard --list

# Generate a vendor plugin
mboot-plugin-wizard --type vendor --name MyVendor --output ./my-vendor-plugin

# Generate a protocol plugin
mboot-plugin-wizard --type protocol --name MyProtocol --output ./my-protocol-plugin
```

Available templates:
- **Vendor** — Vendor plugin skeleton with IPlugin interface
- **Protocol** — Protocol plugin skeleton with IProtocolPlugin interface
- **Transport** — Transport backend skeleton with ITransport interface
- **Workflow** — Workflow step skeleton
- **Job** — Job operation skeleton
- **Package** — Package format skeleton
- **Discovery** — Discovery module skeleton

Each template produces:
- Full `CMakeLists.txt` with `find_package(MBootCore)`
- Complete C++ header with proper namespace and class skeletons
- Method signatures with default implementations

### Creating a Dynamically Loadable Plugin

For plugins built as shared libraries (`.so` / `.dll` / `.dylib`), include `PluginABI.hpp` and export the three C-linkage ABI functions:

```cpp
#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/PluginABI.hpp>

class MyVendorPlugin : public plugin::IPlugin {
    // ... IPlugin implementation ...
};

extern "C" {

MBOOTCORE_PLUGIN_EXPORT IPlugin* mbootcore_plugin_create() {
    return new MyVendorPlugin();
}

MBOOTCORE_PLUGIN_EXPORT void mbootcore_plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

MBOOTCORE_PLUGIN_EXPORT uint32_t mbootcore_plugin_abi_version() {
    return PluginABIVersion;  // from PluginABI.hpp
}

}
```

The `PluginManager` checks the ABI version before loading. If the plugin's ABI version does not match the host's `PluginABIVersion`, the load is rejected with `PluginIncompatible`.

### Manual Plugin Creation (Static)

```cpp
// my_vendor_plugin.hpp
#include <sdk/VendorSDK.hpp>

using namespace mbootcore::sdk;

class MyVendorPlugin : public plugin::IPlugin {
public:
    plugin::PluginMetadata metadata() const override {
        PluginMetadata meta;
        meta.name = "MyVendorPlugin";
        meta.version = "1.0.0";
        meta.vendor = "My Vendor Inc.";
        meta.description = "Example vendor plugin for demonstration";
        return meta;
    }

    Result<void> initialize(plugin::PluginContext& context) override {
        // Register vendor-specific components
        return Result<void>::Ok();
    }

    Result<void> shutdown() override {
        return Result<void>::Ok();
    }

    plugin::PluginState state() const override { return state_; }
    bool enabled() const override { return enabled_; }

private:
    plugin::PluginState state_ = plugin::PluginState::Loaded;
    bool enabled_ = false;
};
```

## Plugin Lifecycle

```
Loaded → Initialized → Enabled ←→ Disabled → Shutdown → Unloaded
```

### Lifecycle Methods

| Method | When Called | What to Do |
|--------|-------------|------------|
| `initialize(context)` | On load | Register components, acquire resources |
| `registerComponents()` | After init | Register with framework registries |
| `setEnabled(true/false)` | Runtime | Enable/disable active processing |
| `shutdown()` | On unload | Release resources, unregister components |
| `unregisterComponents()` | Before shutdown | Remove from framework registries |

## Plugin Types

### Protocol Plugin

```cpp
class MyProtocolPlugin : public plugin::IProtocolPlugin {
public:
    discovery::ProtocolType protocolType() const override {
        return discovery::ProtocolType::Custom;
    }

    std::vector<discovery::ProtocolType> supportedProtocols() const override {
        return {discovery::ProtocolType::Custom};
    }

    // + IPlugin methods
};
```

### Vendor Plugin

```cpp
class MyVendorPlugin : public plugin::IVendorPlugin {
public:
    discovery::Vendor vendor() const override {
        return discovery::Vendor::Custom;
    }

    std::vector<VidPidEntry> vidPidTable() const override {
        return {{0x1234, 0x5678, "My Device"}};
    }

    std::vector<std::string> knownChipsets() const override {
        return {"MYCHIP-1000", "MYCHIP-2000"};
    }

    std::vector<discovery::BootMode> bootModes() const override {
        return {discovery::BootMode::Download};
    }

    // + IPlugin methods
};
```

## Building a Plugin

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyVendorPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(mbootcore REQUIRED)

add_library(myvendorplugin MODULE
    src/my_vendor_plugin.cpp
)

target_link_libraries(myvendorplugin PRIVATE
    mbootcore::mbootcore
)

target_include_directories(myvendorplugin PRIVATE
    ${mbootcore_INCLUDE_DIRS}
)

install(TARGETS myvendorplugin
    LIBRARY DESTINATION lib/mbootcore/plugins
)
```

## Using the Vendor SDK Builder

For more complex plugins, use the `VendorSDK` builder pattern:

```cpp
#include <sdk/VendorSDK.hpp>

mbootcore::sdk::VendorSDK sdk;

// Register components
VendorRegistration vendor;
vendor.name = "MyVendor";
vendor.displayName = "My Vendor Inc.";
vendor.vendorId = discovery::Vendor::Custom;
sdk.registerVendor(vendor);

// Register protocol
ProtocolRegistration proto;
proto.name = "MyProtocol";
proto.vendor = "MyVendor";
proto.transports = {"usb"};
sdk.registerProtocol(proto);

// Finalize and validate
auto report = sdk.finalize();
if (report.valid) {
    // SDK is ready for deployment
}
```

## Testing Plugins

Plugins can be tested using the PluginManager with static or dynamic loading:

### Static Loading

```cpp
#include <mbootcore/plugin/PluginManager.hpp>
#include <myvendor/MyVendorPlugin.hpp>

mbootcore::plugin::PluginManager manager;

// Create plugin instance
auto plugin = std::make_unique<MyVendorPlugin>();

// Load plugin (static loading — no shared library loading)
PluginConfig config;
config.autoInitialize = true;
auto result = manager.load(std::move(plugin), config);
assert(result.isOk());

// Initialize
manager.initialize("MyVendorPlugin");
assert(manager.pluginState("MyVendorPlugin") == PluginState::Initialized);

// Find by type
auto* proto = manager.findProtocolPlugin(ProtocolType::Custom);
assert(proto != nullptr);

// Cleanup
manager.shutdownAll();
manager.unload("MyVendorPlugin");
```

### Dynamic Loading (from .so)

```cpp
#include <mbootcore/plugin/PluginManager.hpp>

mbootcore::plugin::PluginManager manager;

// Load plugin from shared library
auto result = manager.loadFromFile("/path/to/libMyVendor.so");
assert(result.isOk());
assert(manager.pluginState("MyVendorPlugin") == PluginState::Loaded);

// Initialize and use
manager.initialize("MyVendorPlugin");

// Load all plugins from a directory
auto dirResult = manager.loadAllFromDirectory("/usr/lib/mbootcore/plugins");

// Cleanup
manager.shutdownAll();
// Dynamic plugins are automatically unloaded when PluginManager is destroyed,
// closing the shared library handles via RAII.
```

## Compatibility

Ensure your plugin's compatibility version matches the installed SDK:

```cpp
// Plugin declares compatibility with SDK versions 1.0 to 2.0
PluginDependency sdkDep = {"MBootCore", ">=1.0.0 <2.0.0", true};
```

The PluginManager checks:
- SDK version compatibility (accepts 1–2)
- Runtime version range
- Protocol/transport availability
- Dependency graph (no circular deps)
- ABI compatibility
