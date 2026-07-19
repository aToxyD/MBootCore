# GUI Components

## Framework

### ApplicationController
Orchestrates application lifecycle: initialization, manager creation, shutdown.

### MainWindow
Central window with QStackedWidget for switching between views. Contains menu bar, toolbar, status bar, central stacked widget, dock widget support.

### ThemeManager
Manages Dark, Light, Classic, High Contrast themes with Fusion style and custom QPalette.

### SettingsManager
Persistent configuration via QSettings (INI format).

### WindowStateManager
Dock widget layout persistence with named presets.

### RecentFilesManager
Tracks recently opened files with timestamps.

### ActionManager
Central action registry: create, find, enable/disable.

### ShortcutManager
Keyboard shortcut management with persistence.

### CommandManager
Undo/redo stack for user commands.

### NotificationCenter
Cross-component notification system with severity levels.

## Discovery

- **DeviceDiscoveryWidget** — Main discovery panel with search, refresh, auto-refresh
- **DeviceListModel** — QAbstractListModel for flat device list (15 roles)
- **DeviceTreeModel** — QAbstractItemModel, groups by vendor → protocol → device
- **DeviceFilterProxyModel** — QSortFilterProxyModel with text/vendor/protocol/status filters
- **DeviceDetailsWidget** — Selected device properties with connect/disconnect

## Session

- **SessionWidget** — Connect/disconnect/pause/resume/cancel controls
- **SessionMonitor** — Real-time log display with level filtering and search
- **SessionStatistics** — Read/write bytes, transfer speed, elapsed time, errors

## Firmware

- **PackageExplorer** — Split view with tree navigation and detail panels
- **FirmwareInspector** — Package metadata: name, version, size, type, hash, signature, compression
- **MetadataViewer** — Table view of firmware metadata key-value pairs
- **PartitionViewer** — Table view of partitions with names and sizes
- **DependencyViewer** — Tree view of firmware dependencies
- **IntegrityChecker** — Widget that displays "Not verified" status; hash and signature fields show "-" (no cryptographic backend configured)

## Flash

- **FlashWidget** — Flash operation panel with plan loading, start/stop/pause/resume
- **FlashPlanViewer** — Tree view of flash plan structure
- **PartitionSelection** — Checkable table for partition selection
- **ProgressWindow** — Progress bar, status, operation name, estimated time
- **FlashStatistics** — Bytes transferred, speed, retries, errors, warnings, elapsed

## GPT

- **PartitionTableWidget** — GPT management with viewer, editor, backup/restore/compare
- **GPTViewer** — Table view of GPT partition entries
- **PartitionEditor** — Edit partition name, GUID, start/end sectors, type
- **BackupDialog / RestoreDialog / CompareDialog** — GPT backup, restore, comparison
