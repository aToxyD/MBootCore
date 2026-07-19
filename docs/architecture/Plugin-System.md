# Plugin System

## Overview

The Plugin SDK provides a zero-dependency extension framework for adding new protocols, vendors, discovery strategies, pipeline stages, and other capabilities to MBootCore — without modifying core library code or including any protocol headers (Sahara, Firehose, ELF, GPT, Loader).

## Architecture

```
┌──────────────────────────────────────────────────┐
│                    Application                    │
└──────────────────┬───────────────────────────────┘
                   │
┌──────────────────▼───────────────────────────────┐
│                 PluginManager                     │
│  load/unload/initialize/shutdown/enable/disable  │
│  findPlugin / findProtocolPlugin / findVendorPlugin
│  resolveDependencies / reload / batch operations │
└────┬──────────────┬──────────────┬───────────────┘
     │              │              │
     ▼              ▼              ▼
┌──────────┐ ┌────────────┐ ┌────────────┐
│ IPlugin  │ │IProtocolPlugin│IVendorPlugin│
└──────────┘ └────────────┘ └────────────┘
     │              │              │
     └──────┬───────┴──────┬───────┘
            │              │
            ▼              ▼
     ┌────────────┐ ┌──────────────┐
     │PluginContext│ │PluginTypes   │
     └────────────┘ └──────────────┘
            │
            ▼
     ┌──────────────────────────────────────────────┐
     │        Core Framework (registry, engines)     │
     └──────────────────────────────────────────────┘
```

## Key Design Principle

**Zero protocol headers in the SDK.** Plugin interfaces depend only on:
- `PluginTypes.hpp` — enums, structs, config
- `PluginContext.hpp` — access to framework services via forward-declared types
- `Error.hpp` — `Result<T>` return type

No protocol, ELF, GPT, Loader, or Pipeline header is ever included by the plugin SDK.

## ABI for Dynamic Loading

Plugins built as shared libraries (`.so` / `.dll` / `.dylib`) export three C-linkage functions defined in `PluginABI.hpp`:

```cpp
extern "C" {
    MBOOTCORE_PLUGIN_EXPORT IPlugin* mbootcore_plugin_create();
    MBOOTCORE_PLUGIN_EXPORT void     mbootcore_plugin_destroy(IPlugin*);
    MBOOTCORE_PLUGIN_EXPORT uint32_t mbootcore_plugin_abi_version();
}
```

| Symbol | Purpose |
|--------|---------|
| `mbootcore_plugin_create` | Factory: returns a new `IPlugin*` instance |
| `mbootcore_plugin_destroy` | Destroys an `IPlugin*` created by `mbootcore_plugin_create` |
| `mbootcore_plugin_abi_version` | Returns `PluginABIVersion` for compatibility checking |

### ABI Constants (`PluginABI.hpp`)

```cpp
namespace mbootcore::plugin {
    constexpr uint32_t PluginABIVersion = 1;
    constexpr const char* PluginCreateSymbol  = "mbootcore_plugin_create";
    constexpr const char* PluginDestroySymbol = "mbootcore_plugin_destroy";
    constexpr const char* PluginVersionSymbol = "mbootcore_plugin_abi_version";
}
```

### Export Macro

Use `MBOOTCORE_PLUGIN_EXPORT` on the three ABI functions. The macro resolves to:
- Windows: `__declspec(dllexport)` (when building) / `__declspec(dllimport)` (when consuming)
- GCC/Clang: `__attribute__((visibility("default")))`
- Other: empty

## Plugin Lifecycle

```
Loaded → Initialized → Enabled ←→ Disabled → Shutdown → Unloaded
```

| State | Description |
|-------|-------------|
| Loaded | Plugin loaded into registry, not yet initialized |
| Initialized | Plugin initialized, services registered |
| Enabled | Plugin actively processing |
| Disabled | Plugin inactive, services not available |
| Shutdown | Plugin shutdown, cleanup complete |
| Unloaded | Plugin removed from registry |

### Lifecycle Methods

```cpp
class IPlugin {
    Result<Unit> initialize(PluginContext& context);
    Result<Unit> shutdown();
    Result<Unit> registerComponents();
    Result<Unit> unregisterComponents();
    PluginState state() const;
    bool enabled() const;
    Result<Unit> setEnabled(bool enabled);
    PluginMetadata metadata() const;
};
```

## Plugin Types

### Basic Plugin (`IPlugin`)

Base interface for all plugins. Provides lifecycle management and component registration.

### Protocol Plugin (`IProtocolPlugin`)

Extends `IPlugin` for implementing new boot protocols:

```cpp
class IProtocolPlugin : public IPlugin {
    ProtocolType protocolType() const;
    std::vector<ProtocolType> supportedProtocols() const;
};
```

### Vendor Plugin (`IVendorPlugin`)

Extends `IPlugin` for adding vendor-specific capabilities:

```cpp
class IVendorPlugin : public IPlugin {
    Vendor vendor() const;
    std::vector<VidPidEntry> vidPidTable() const;
    std::vector<std::string> knownChipsets() const;
    std::vector<BootMode> bootModes() const;
};
```

## PluginManager

The `PluginManager` provides comprehensive plugin management:

| Operation | Description |
|-----------|-------------|
| `load(plugin, config)` | Load a plugin from an `std::unique_ptr<IPlugin>` instance |
| `loadFromFile(filePath)` | Load a plugin from a shared library (`.so`/`.dll`/`.dylib`) |
| `loadAllFromDirectory(dir)` | Load all `.so`/`.dll` files from a directory |
| `unload(name)` | Unload a plugin |
| `initialize(name)` | Initialize a specific plugin |
| `initializeAll()` | Initialize all loaded plugins |
| `shutdown(name)` | Shutdown a specific plugin |
| `shutdownAll()` | Shutdown all plugins |
| `enable(name)` / `disable(name)` | Toggle active state |
| `findPlugin(name)` | Find by name using `dynamic_cast` |
| `findProtocolPlugin(type)` | Find by protocol type |
| `findVendorPlugin(vendor)` | Find by vendor |

### Dependency Management

The PluginManager maintains a dependency graph for automatic resolution:

- `resolveDependencies()` — validates all dependency chains
- Circular dependency detection
- Optional vs required dependency handling
- Version compatibility verification (accepts SDK versions 1–2)

### Batch Operations

```cpp
manager.loadAll(std::move(plugins));  // plugins: std::vector<std::unique_ptr<IPlugin>>
manager.initializeAll();
// ... use plugins ...
manager.shutdownAll();
```

## Plugin Components

### PluginTypes

Core type definitions:

| Type | Description |
|------|-------------|
| `PluginState` | Lifecycle state enum |
| `PluginCapability` | Bit-flag capability flags |
| `PluginDependency` | Name, version range, required flag |
| `PluginMetadata` | Name, version, author, description, dependencies (immutable) |
| `PluginConfig` | Runtime-configurable settings (mutable) |

### PluginContext

Provides access to framework services:

- `DeviceManager*` — session management
- `ProtocolRegistry*` — protocol registration
- `DeviceDiscoveryEngine*` — device discovery
- `ProtocolNegotiationEngine*` — protocol negotiation
- `ILogger*` — logging

### Plugin Compatibility

| Check | Description |
|-------|-------------|
| SDK version | Plugin SDK version vs installed SDK version |
| Runtime version | Plugin runtime requirements vs installed runtime |
| Protocol compatibility | Protocol type availability in registry |
| Transport compatibility | Transport backend availability |
| Dependency resolution | All required dependencies available and compatible |

### PluginManifest

Manages plugin metadata with JSON serialization:

```cpp
PluginManifest manifest;
manifest.fromJson(jsonString);
bool valid = manifest.isValid();
auto errors = manifest.validate();
std::string json = manifest.toJson();
```

## Plugin SDK

The SDK (`sdk/include/sdk/`) provides 19 public headers for plugin development:

| Header | Purpose |
|--------|---------|
| `SDKInfo.hpp` | SDK name, version, build info, SDKDoctor diagnostics |
| `Version.hpp` | Semantic versioning type |
| `VendorSDK.hpp` | Vendor plugin builder interface |
| `VendorSDKFactory.hpp` | Factory for creating VendorSDK instances |
| `PluginManifest.hpp` | Plugin metadata management with JSON serialization |
| `PluginMetadata.hpp` | Extended plugin metadata types |
| `PluginValidator.hpp` | Registration validation |
| `PluginCompatibility.hpp` | Plugin compatibility reporting |
| `PluginDependencyGraph.hpp` | Plugin dependency management |
| `APICompatibility.hpp` | API compatibility checks |
| `SDKValidator.hpp` | SDK installation validator |
| `VendorRegistration.hpp` | Vendor registration structures |
| `ProtocolRegistration.hpp` | Protocol registration structures |
| `TransportRegistration.hpp` | Transport registration structures |
| `WorkflowRegistration.hpp` | Workflow registration structures |
| `JobRegistration.hpp` | Job registration structures |
| `PackageRegistration.hpp` | Package format registration structures |
| `DiscoveryRegistration.hpp` | Discovery module registration structures |
| `CapabilityRegistration.hpp` | Capability registration structures |

## Vendor SDK

The Vendor SDK provides a builder pattern for creating vendor plugins:

```cpp
#include <sdk/VendorSDK.hpp>

mbootcore::sdk::VendorSDK sdk;

VendorRegistration vendor;
vendor.name = "MyVendor";
vendor.displayName = "My Vendor Inc.";
vendor.vendorId = discovery::Vendor::Custom;
sdk.registerVendor(vendor);

auto report = sdk.finalize();
if (report.valid) {
    // SDK is ready for deployment
}
```

Registration types:
- Vendor — chip vendor registration
- Protocol — protocol registration with transport and vendor compatibility
- Transport — transport backend registration
- Workflow — workflow step registration
- Job — job operation registration
- Package — firmware package format registration
- Discovery — discovery module registration
- Capability — custom capability registration

## Template System

MBootCore provides template generators for creating skeleton plugin projects:

| Template | Generator | Description |
|----------|-----------|-------------|
| Vendor | `VendorTemplateGenerator` | Vendor plugin skeleton |
| Protocol | `ProtocolTemplateGenerator` | Protocol plugin skeleton |
| Transport | `TransportTemplateGenerator` | Transport backend skeleton |
| Workflow | `WorkflowTemplateGenerator` | Workflow step skeleton |
| Job | `JobTemplateGenerator` | Job operation skeleton |
| Package | `PackageTemplateGenerator` | Package format skeleton |
| Discovery | `DiscoveryTemplateGenerator` | Discovery module skeleton |

Each generator produces complete C++ header files with proper include directives, namespace declarations, class skeletons with method signatures, and default implementations.
