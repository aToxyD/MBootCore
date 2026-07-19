# Plugin Creation

Demonstrates:
- Implementing `IPlugin` and `IProtocolPlugin` interfaces
- Creating a custom Demo protocol plugin with metadata
- Registering and unregistering plugin components
- Installing plugins via `Runtime::installPlugin()`
- Listing installed plugins
- Removing plugins via `Runtime::removePlugin()`

## Build

```bash
cd build
cmake --build . --target example_plugin_creation
```

## Run

```bash
./example_plugin_creation
```
