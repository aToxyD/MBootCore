# Environment Variables

MBootCore reads the following environment variables.

## Application

| Variable | Read By | Description |
|----------|---------|-------------|
| `MBOOT_CONFIG` | mboot-cli | Inline configuration string (JSON) |
| `MBOOT_CONFIG_FILE` | mboot-cli | Path to configuration file |

## SDK

| Variable | Read By | Description |
|----------|---------|-------------|
| `MBOOTCORE_SDK_DIR` | SDKDoctor | Root directory of the MBootCore SDK installation |

## System (Windows Only)

| Variable | Read By | Description |
|----------|---------|-------------|
| `ProgramFiles` | SDKDoctor | Standard Program Files directory |
| `LOCALAPPDATA` | SDKDoctor | Local AppData directory |

## Notes

- `MBOOT_CONFIG` and `MBOOT_CONFIG_FILE` are read via `safeGetenv()`, a
  platform-safe wrapper around `GetEnvironmentVariableA` (Windows) and
  `std::getenv` (POSIX).
- Environment variables are read at the application layer, not by the core
  `ConfigManager`. The core config system reads only from in-memory defaults,
  JSON files, and JSON string imports.

## See Also

- [Configuration](../user-guide/Configuration.md) — configuration system
- [Config Reference](ConfigReference.md) — all configuration keys
