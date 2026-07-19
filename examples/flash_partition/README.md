# Flash Partition

Demonstrates:
- Creating a default Runtime via `RuntimeFactory::createDefault()`
- Initializing the Runtime
- Flashing a firmware package via `Runtime::flash()` with progress callbacks
- Proper lifecycle management (initialize → flash → shutdown)

## Build

```bash
cd build
cmake --build . --target example_flash_partition
```

## Run

```bash
./example_flash_partition /path/to/firmware/
```
