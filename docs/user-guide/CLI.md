# CLI User Guide

MBoot CLI (`mboot-cli`) is the command-line interface for MBootCore. It
supports both interactive and one-shot modes.

## Starting the CLI

```bash
# Interactive mode (default)
./mboot-cli

# One-shot command
./mboot-cli discover
./mboot-cli flash firmware.pkg --vendor qualcomm
```

Interactive mode shows a `mboot>` prompt with command history.

## Typical Workflow

```
discover → connect → flash → verify → disconnect
```

```bash
# 1. Scan for devices (5-second timeout)
discover

# 2. Connect to the first device
connect 0

# 3. Flash a firmware package
flash firmware.pkg

# 4. Disconnect
disconnect
```

## Common Commands

| Command | Usage | Description |
|---------|-------|-------------|
| `discover` | `discover` | Scan for connected devices (5s timeout) |
| `connect` | `connect <index>` | Connect to a device by index |
| `disconnect` | `disconnect` | Disconnect from current device |
| `flash` | `flash <file>` | Flash a firmware package |
| `read` | `read <addr> <size>` | Read device memory |
| `write` | `write <addr> <hex>` | Write hex data to device memory |
| `erase` | `erase <addr> [size]` | Erase device memory (default 4096 bytes) |
| `verify` | `verify <addr> <hex>` | Verify device memory against expected data |
| `backup` | `backup <partition> [file]` | Backup a partition to file |
| `restore` | `restore <partition> <file>` | Restore a partition from file |
| `package` | `package info <path>` | Show firmware package info |
| `plugin` | `plugin` | List loaded plugins |
| `vendor` | `vendor` | List registered vendors |
| `session` | `session` | Show session status |
| `statistics` / `stats` | `statistics` | Show runtime statistics |
| `health` | `health` | Show system health |

## Dry Run

Simulate a flash without writing to the device:

```bash
flash firmware.pkg --dry-run
```

## Output Formats

```bash
# Default human-readable output
discover

# JSON output
discover --json

# XML output
discover --xml

# Quiet mode (errors only)
discover --quiet
```

## Interactive Mode

```bash
./mboot-cli
mboot> discover
mboot> connect 0
mboot> flash firmware.pkg
mboot> exit
```

Lines starting with `#` or `;` are comments:

```
# Flash the firmware
flash firmware.pkg
```

## Scripts

Execute a batch file:

```bash
./mboot-cli script flash-all.txt
```

Script files are executed line-by-line. Lines starting with `#` are skipped.

## Shell Completion

Generate tab-completion scripts:

```bash
./mboot-cli completion bash    # Bash
./mboot-cli completion zsh     # Zsh
./mboot-cli completion powershell  # PowerShell
```

## See Also

- [CLI Reference](../reference/CLIReference.md) — exhaustive command and option reference
- [Error Codes](../reference/ErrorCodes.md) — complete error code listing
- [Installation](../getting-started/Installation.md) — setup instructions
