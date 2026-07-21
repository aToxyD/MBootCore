# Configuration Reference

All configuration keys in `mbootcore::config::FullConfig`. Extracted directly
from `lib/include/mbootcore/config/ConfigTypes.hpp`.

## RuntimeConfig

| Key | Type | Default |
|-----|------|---------|
| `runtime.threadPoolSize` | uint32 | `4` |
| `runtime.operationTimeoutMs` | uint32 | `30000` |
| `runtime.enableLogging` | bool | `true` |
| `runtime.enableTelemetry` | bool | `false` |
| `runtime.enableProfiling` | bool | `false` |
| `runtime.logLevel` | string | `"info"` |
| `runtime.tempDirectory` | string | `"/tmp/mbootcore"` |

## TransportConfig

| Key | Type | Default |
|-----|------|---------|
| `transport.usbTimeoutMs` | uint32 | `5000` |
| `transport.serialTimeoutMs` | uint32 | `5000` |
| `transport.tcpTimeoutMs` | uint32 | `10000` |
| `transport.maxRetries` | uint32 | `3` |
| `transport.enableKeepAlive` | bool | `true` |
| `transport.keepAliveIntervalMs` | uint32 | `1000` |
| `transport.bufferSize` | uint32 | `65536` |

## WorkflowConfig

| Key | Type | Default |
|-----|------|---------|
| `workflow.maxRetries` | uint32 | `3` |
| `workflow.retryDelayMs` | uint32 | `1000` |
| `workflow.enableRollback` | bool | `true` |
| `workflow.enableCheckpoints` | bool | `true` |
| `workflow.enableValidation` | bool | `true` |
| `workflow.maxParallelStages` | uint32 | `4` |

## JobConfig

| Key | Type | Default |
|-----|------|---------|
| `job.maxRetries` | uint32 | `3` |
| `job.retryDelayMs` | uint32 | `1000` |
| `job.maxParallel` | uint32 | `4` |
| `job.enableRecovery` | bool | `true` |
| `job.enableHistory` | bool | `true` |
| `job.historyLimit` | uint32 | `1000` |

## DSPConfig

| Key | Type | Default |
|-----|------|---------|
| `dsp.enableVerification` | bool | `true` |
| `dsp.enableCaching` | bool | `true` |
| `dsp.cacheSizeMb` | uint32 | `256` |
| `dsp.repositoryPath` | string | `"/etc/mbootcore/dsp"` |
| `dsp.trustedVendors` | vector\<string\> | *(empty)* |

## PluginConfig

| Key | Type | Default |
|-----|------|---------|
| `plugin.enableVerification` | bool | `true` |
| `plugin.enableHotReload` | bool | `false` |
| `plugin.maxPlugins` | uint32 | `100` |
| `plugin.pluginPaths` | vector\<string\> | *(empty)* |
| `plugin.blacklist` | vector\<string\> | *(empty)* |

## VendorConfig

| Key | Type | Default |
|-----|------|---------|
| `vendor.enableAutoDetect` | bool | `true` |
| `vendor.preferredVendors` | vector\<string\> | *(empty)* |
| `vendor.vendorOptions` | map\<string,string\> | *(empty)* |

## GUIConfig

| Key | Type | Default |
|-----|------|---------|
| `gui.enableAnimations` | bool | `true` |
| `gui.enableTrayIcon` | bool | `true` |
| `gui.theme` | string | `"dark"` |
| `gui.language` | string | `"auto"` |
| `gui.windowWidth` | uint32 | `1280` |
| `gui.windowHeight` | uint32 | `720` |
| `gui.maximizeOnStart` | bool | `false` |

## CLIConfig

| Key | Type | Default |
|-----|------|---------|
| `cli.enableColor` | bool | `true` |
| `cli.enableProgress` | bool | `true` |
| `cli.outputFormat` | string | `"text"` |
| `cli.verbosity` | uint32 | `1` |

## FullConfig (Aggregate)

| Key | Type | Default |
|-----|------|---------|
| `source` | ConfigSource | `Default` |
| `customOptions` | map\<string,string\> | *(empty)* |

## Config Sources

| Value | Name | Description |
|-------|------|-------------|
| 0 | `Default` | Built-in defaults |
| 1 | `File` | Loaded from JSON file |
| 2 | `Environment` | From environment variables |
| 3 | `API` | Set via programmatic API |
| 4 | `CLI` | Set via command-line arguments |

**Total: 52 configuration keys** (across 9 config structs + 2 aggregate keys)

Source: `lib/include/mbootcore/config/ConfigTypes.hpp`

## See Also

- [Configuration](../user-guide/Configuration.md) — configuration system usage
- [Environment Variables](EnvironmentVariables.md) — environment variable reference
