# Runtime Usage

Demonstrates:
- Full runtime lifecycle with `RuntimeBuilder` and custom `RuntimeConfig`
- Attaching a `RuntimeObserver` to monitor events, statistics, and health
- Version info and SDK metadata
- Hardware diagnostics and USB/Serial enumeration
- Capability listing
- Device discovery
- Plugin listing
- Pause/Resume/Cancel/Reset control operations
- Clean shutdown

## Build

```bash
cd build
cmake --build . --target example_runtime_usage
```

## Run

```bash
./example_runtime_usage
```
