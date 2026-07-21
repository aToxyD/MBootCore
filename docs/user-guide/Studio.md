# MBoot Studio User Guide

MBoot Studio is the Qt-based GUI application for MBootCore. It provides a
visual interface for device discovery, firmware flashing, partition management,
and workflow design.

## Launching

```bash
./mboot-studio
```

Qt 6.5+ is required. If Qt is not installed, Studio is not built. The core
library and CLI are available without Qt.

## Main Window

- **Default size:** 1280×800 (minimum 1024×600)
- **Toolbar:** Quick access to Discovery, Session, Firmware, Flash, GPT, Workflow, Jobs
- **Status bar:** Current status (left), device count (right), connection state (right)

## Views

Switch views via the View menu, toolbar, or keyboard shortcuts:

| Shortcut | View | Description |
|----------|------|-------------|
| Ctrl+1 | Device Discovery | Scan and connect to devices |
| Ctrl+2 | Session | Manage device sessions |
| Ctrl+3 | Firmware Explorer | Browse firmware packages |
| Ctrl+4 | Flash Operations | Flash firmware to devices |
| Ctrl+5 | GPT Partition Manager | View and manage partition tables |
| Ctrl+6 | Workflow Designer | Design and execute workflows |
| Ctrl+7 | Job Queue | View and manage queued jobs |
| Ctrl+8 | Plugin Manager | Manage installed plugins |
| Ctrl+9 | Vendor Manager | Manage vendor integrations |
| Ctrl+0 | Transport Monitor | Monitor USB/Serial/TCP statistics |

Additional views (no shortcut): Log Viewer, Diagnostics, Developer Tools.

## Status Bar

| Label | Meaning |
|-------|---------|
| Ready / Operation name | Current application status |
| No device / N devices | Number of detected devices |
| Disconnected / Connected: name | Device connection state |

## Common Workflows

### Discover and Flash

1. **Ctrl+1** → Device Discovery → click "Discover"
2. Select a device from the list → click "Connect"
3. **Ctrl+4** → Flash Operations → select firmware file
4. Click "Flash" and monitor progress

### Inspect Partitions

1. **Ctrl+1** → Connect to a device
2. **Ctrl+5** → GPT Partition Manager → view partition table
3. Use Backup/Restore dialogs for individual partitions

### Monitor Transport

1. **Ctrl+0** → Transport Monitor
2. View USB, Serial, and TCP connection statistics

## Menu Reference

**File:** About, Quit  
**View:** All views listed above  
**Tools:** Discover Devices (F5)  
**Help:** Check for Updates

## See Also

- [CLI User Guide](CLI.md) — command-line interface
- [GUI Architecture](../gui/Architecture.md) — GUI layer structure
- [GUI Components](../gui/Components.md) — widget catalog
- [Keyboard Shortcuts](../gui/Shortcuts.md) — complete shortcut reference
