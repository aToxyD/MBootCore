# CLI Embedding

Demonstrates:
- Embedding a simple CLI argument parser
- Handling `--help`, `--version`, `--discover`, `--info`, `--diagnose` flags
- Using `RuntimeFactory::createCLI()` for CLI-optimized runtime
- Running actions: flash, backup, erase
- Using `SDKDoctor` for runtime diagnostics
- Displaying formatted output via `SDKInfo` and `VersionInfo`

## Build

```bash
cd build
cmake --build . --target example_cli_embedding
```

## Run

```bash
# Help
./example_cli_embedding --help

# Version
./example_cli_embedding --version

# SDK info
./example_cli_embedding --info

# Diagnostics
./example_cli_embedding --diagnose

# Discover devices
./example_cli_embedding --discover --timeout 3000

# Flash firmware
./example_cli_embedding --action flash --firmware /path/to/firmware

# Backup partition
./example_cli_embedding --action backup --partition boot
```
