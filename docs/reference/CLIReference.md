# CLI Reference

Exhaustive reference for `mboot-cli` commands, options, and exit codes.

## Commands

### Device Operations

| Command | Arguments | Description |
|---------|-----------|-------------|
| `discover` | *(none)* | Scan for connected devices (5-second timeout) |
| `connect` | `<index>` | Connect to a device by discovery index (0-based) |
| `disconnect` | *(none)* | Disconnect from current device |
| `reconnect` | *(none)* | Reconnect to previously connected device |

### Memory Operations

| Command | Arguments | Description |
|---------|-----------|-------------|
| `read` | `<addr> <size>` | Read device memory at address |
| `write` | `<addr> <hex_data>` | Write hex-encoded data to device memory |
| `erase` | `<addr> [size]` | Erase device memory (default: 4096 bytes) |
| `verify` | `<addr> <expected_hex>` | Verify device memory matches expected data |

### Flash Operations

| Command | Arguments | Description |
|---------|-----------|-------------|
| `flash` | `<file> [--dry-run]` | Flash a firmware package |
| `backup` | `<partition> [output_file]` | Backup a named partition |
| `restore` | `<partition> <input_file>` | Restore a partition from file |

### Package Operations

| Command | Arguments | Description |
|---------|-----------|-------------|
| `package` | `info <path>` | Show firmware package info |

### Workflow and Jobs

| Command | Arguments | Description |
|---------|-----------|-------------|
| `workflow` | `<type>` | Execute a named workflow |
| `job` | `<name> <partition> <file>` | Run a flash job |

### Plugin and Vendor

| Command | Arguments | Description |
|---------|-----------|-------------|
| `plugin` | `[list]` | List loaded plugins |
| `vendor` | `[list]` | List registered vendors |
| `transport` | `[list]` | List available transport IDs |

### Session and Monitoring

| Command | Arguments | Description |
|---------|-----------|-------------|
| `session` | *(none)* | Show active session status |
| `statistics` / `stats` | *(none)* | Show runtime statistics |
| `health` | *(none)* | Show system health |
| `capabilities` | *(none)* | Show runtime capabilities |
| `monitor` | *(none)* | Start monitoring (not available in this version) |

### Control

| Command | Arguments | Description |
|---------|-----------|-------------|
| `reset` | *(none)* | Reset the runtime |
| `cancel` | *(none)* | Cancel current operation |
| `pause` | *(none)* | Pause current operation |
| `resume` | *(none)* | Resume paused operation |
| `reboot` | *(none)* | Reboot connected device |
| `shutdown` | *(none)* | Shut down the runtime |

### Utility

| Command | Arguments | Description |
|---------|-----------|-------------|
| `help` | *(none)* | Show help text |
| `version` | *(none)* | Show version information |
| `list` | *(none)* | List available commands |
| `shell` | *(none)* | Enter interactive mode |
| `script` | `<file>` | Execute a script file |
| `completion` | `<shell>` | Generate shell completion (bash, zsh, powershell) |

## Global Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--device=<id>` | string | — | Specify device by ID |
| `--vendor=<name>` | string | — | Specify vendor |
| `--transport=<type>` | string | — | Specify transport type |
| `--protocol=<type>` | string | — | Specify protocol type |
| `--partition=<name>` | string | — | Specify partition name |
| `--address=<hex\|dec>` | uint64 | `0` | Specify address |
| `--size=<bytes>` | size_t | `0` | Specify size |
| `--timeout=<ms>` | int | `5000` | Operation timeout (ms) |
| `--retry=<n>` | int | `3` | Retry count |
| `--verbose` | flag | `false` | Verbose output |
| `--quiet` | flag | `false` | Quiet output (errors only) |
| `--json` | flag | `false` | JSON output format |
| `--xml` | flag | `false` | XML output format |
| `--no-progress` | flag | `false` | Disable progress display |
| `--force` | flag | `false` | Force operation |
| `--dry-run` | flag | `false` | Simulate only |

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | General/unknown error |
| 2 | Bad arguments (`InvalidArgument`) |
| 3 | Device not found |
| 4 | Transport errors (timeout, disconnect, read/write failure) |
| 5 | Workflow errors (execution, step, recovery failure) |
| 6 | Flash/Firehose errors (program, erase, validation failure) |
| 7 | Verification failed (hash mismatch) |
| 8 | Session timeout |
| 9 | Cancelled |
| 10 | Runtime initialization failure |

## Interactive Mode

- Prompt: `mboot>`
- `exit` or `quit` to leave
- Lines starting with `#` or `;` are comments
- Command history is maintained

## Scripts

Script files are executed line-by-line. Lines starting with `#` are skipped.

```bash
./mboot-cli script flash-all.txt
```

## See Also

- [CLI User Guide](../user-guide/CLI.md) — practical usage guide
- [Error Codes](ErrorCodes.md) — complete error code listing
- [Environment Variables](EnvironmentVariables.md) — environment variable reference
