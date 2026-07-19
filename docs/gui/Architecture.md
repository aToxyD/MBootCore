# GUI Architecture

## Overview

MBoot Studio follows Clean Architecture principles: the GUI contains zero protocol logic and communicates exclusively through the Runtime layer.

## Layers

```
┌─────────────────────────────────────────┐
│           Presentation (Views)           │
│  Widgets, Dialogs, MainWindow, Docks    │
├─────────────────────────────────────────┤
│           ViewModels / Controllers       │
│  Models, Proxies, Managers, Services     │
├─────────────────────────────────────────┤
│              Runtime Layer               │
│  Discovery, Session, Workflow, Jobs     │
├─────────────────────────────────────────┤
│              MBootCore Core             │
│  Sahara, Firehose, ELF, GPT, Transport  │
└─────────────────────────────────────────┘
```

## Key Design

- **No protocol code in GUI** — all protocol logic stays in the core library
- **Runtime as sole backend** — GUI uses only Runtime APIs
- **Fully asynchronous** — long operations use Qt Concurrent, signals, worker threads
- **Model/View architecture** — all data displayed through Qt Model/View
- **Dockable windows** — all panels are QDockWidget-based, with savable/restorable layouts
- **Persistent settings** — geometry, recent files, themes, shortcuts via QSettings

## Component Map

| Package | Purpose |
|---------|---------|
| `runtime/` | RuntimeBridge, event dispatch, error mapping, converters, models |
| `framework/` | Application shell: MainWindow, managers, controllers |
| `discovery/` | Device discovery UI: lists, trees, filters, details |
| `session/` | Session management: tabs, monitor, statistics |
| `firmware/` | Package browser: inspector, metadata, partitions |
| `flash/` | Flash operations: plan viewer, progress, statistics |
| `gpt/` | Partition table: viewer, editor, backup/restore |
| `workflow/` | Workflow designer: graph, steps, recovery |
| `job/` | Job queue: history, scheduler, progress |
| `plugin/` | Plugin manager: load, enable, disable |
| `vendor/` | Vendor manager: info, capabilities |
| `transport/` | Transport monitor: USB/serial/TCP, statistics |
| `logs/` | Log viewer: filtering, search, export |
| `settings/` | Settings dialog: all configuration tabs |
| `diagnostics/` | Diagnostics: runtime, transport, environment |
| `devtools/` | Developer tools: inspectors, debuggers |
| `theme/` | Theme preview and management |
| `l10n/` | Localization support |

## Communication

1. **Widget → RuntimeBridge**: Widgets access Runtime exclusively through `RuntimeBridge`
2. **RuntimeBridge → Runtime**: Thread-safe calls to `mbootcore::runtime::Runtime` via PIMPL
3. **Runtime → Widget**: `RuntimeEventDispatcher` translates Core events to Qt signals via queued connections
4. **Model → View**: Qt signals (dataChanged, rowsInserted, etc.)
5. **Cross-widget**: Through NotificationCenter, CommandManager, or shared models

### Dependency Direction

```
Qt Widgets
        ↓
RuntimeBridge
        ↓
Runtime
        ↓
Diagnostics Service / Plugin Manager / Core Services
```

### Diagnostics Flow

```
DiagnosticsWidget
        ↓ setRuntimeBridge()
RuntimeBridge::refreshDiagnostics()
        ↓ QtConcurrent
Runtime::health() + statistics() + hardwareReport()
        ↓
RuntimeConverters::fromCoreDiagnostics()
        ↓ QMetaObject::invokeMethod (QueuedConnection)
RuntimeEventDispatcher::diagnosticsUpdated signal
        ↓
DiagnosticsWidget::onDiagnosticsUpdated()
```

### Plugin Flow

```
PluginManagerWidget
        ↓ setRuntimeBridge()
RuntimeBridge::enumeratePlugins()
        ↓
Runtime::listPlugins()
        ↓
RuntimeConverters::fromPluginName() + fromCapabilityString()
        ↓ QMetaObject::invokeMethod (QueuedConnection)
RuntimeEventDispatcher::pluginListChanged signal
        ↓
PluginManagerWidget::onPluginListChanged()
```

### Capability Flow

```
RuntimeBridge::runtimeCapabilities()
        ↓
Runtime::capabilities()
        ↓
RuntimeConverters::fromCapabilityString()
        ↓ QMetaObject::invokeMethod (QueuedConnection)
RuntimeEventDispatcher::capabilitiesChanged signal
        ↓
PluginManagerWidget::onCapabilitiesChanged()
```

Never:

```
Widget
    ↓
Runtime
```

### Architecture Rules

- **RuntimeBridge** is the only class allowed to own or communicate with Runtime
- **RuntimeBridge** implements `RuntimeObserver` and forwards events to `RuntimeEventDispatcher`
- **RuntimeEventDispatcher** translates Core events into Qt signals via `QMetaObject::invokeMethod` with `Qt::QueuedConnection`
- **RuntimeErrorMapper** converts `Result<T>`/`ErrorInfo` into GUI-friendly `RuntimeError` objects
- **RuntimeConverters** provide explicit Core-to-GUI type conversions
- **RuntimeModels** define lightweight presentation models independent of Core implementation
- Widgets never instantiate Runtime directly
- Widgets never include Core headers except through the bridge layer
- All long-running operations (discovery, connect, disconnect, flash, firmware load, diagnostics refresh) execute asynchronously via `QtConcurrent` in `RuntimeBridge::Impl`
- Widgets connect to `RuntimeEventDispatcher` signals for live data updates
- Flash workflow uses `FlashOperationView`/`FlashProgressView`/`FlashResultView` models
- Flash cancellation propagates through `RuntimeBridge::cancelFlash()` → `Runtime::cancel()`
- Diagnostics flow uses `RuntimeDiagnosticsView`/`RuntimeHealthView`/`DeviceHealthView` models
- Plugin management uses `PluginInfoView`/`CapabilityView` models
- Plugin enable/disable requires direct `PluginManager` access (not available through public `IPluginService` API)

## State

- `ApplicationController` owns all managers, MainWindow, and `RuntimeBridge`
- `RuntimeBridge` owns `mbootcore::runtime::Runtime` via `unique_ptr`
- `SettingsManager` provides persistent key-value storage via QSettings
- `WindowStateManager` saves/restores dock layout and window geometry
- `CommandManager` provides undo/redo for user actions
- `NotificationCenter` provides cross-component messaging
