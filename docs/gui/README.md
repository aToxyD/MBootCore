# MBoot Studio

Qt 6 desktop GUI for MBootCore, a professional low-level C++17 framework for BootROM protocols.

## Quick Start

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -j$(nproc)
```

### Dependencies

- Qt 6.x (Widgets)
- C++17 compiler (GCC, Clang, MinGW-w64, AppleClang)
- MBootCore library (from lib/)

## Architecture

MBoot Studio follows a layered Clean Architecture:

```
Qt Widgets (mboot-studio)
        |
RuntimeBridge (PIMPL, owns Runtime)
        |
Runtime (MBootCore)
        |
Core Services (Discovery, Session, Plugin, Transport)
```

### Key Principles

- Core has no Qt dependency: No Qt includes in lib/ headers
- RuntimeBridge is the sole communication layer: Widgets never access Runtime directly
- Thread safety: All long-running operations via QtConcurrent
- Event-driven: RuntimeObserver to RuntimeEventDispatcher to Qt signals

### Runtime Infrastructure

| Component | Header | Purpose |
|-----------|--------|---------|
| RuntimeBridge | runtime/RuntimeBridge.hpp | PIMPL wrapper around Runtime, thread-safe async operations |
| RuntimeEventDispatcher | runtime/RuntimeEventDispatcher.hpp | Core events to Qt signals via QueuedConnection |
| RuntimeErrorMapper | runtime/RuntimeErrorMapper.hpp | Result<T> to GUI-friendly RuntimeError |
| RuntimeConverters | runtime/RuntimeConverters.hpp | Core-to-GUI type conversions |
| RuntimeModels | runtime/RuntimeModels.hpp | Presentation models (DeviceInfoView, SessionInfoView, etc.) |
| RuntimeTypes | runtime/RuntimeTypes.hpp | Enums (FlashStatus, DiagnosticSeverity, PluginStatus, etc.) |

## Widget Hierarchy

### Central Views (QStackedWidget)

| View | Widget | Description |
|------|--------|-------------|
| Discovery | DeviceDiscoveryWidget | Detect and filter connected devices |
| Session | SessionWidget | Manage device sessions, connect/disconnect |
| Firmware | PackageExplorer | Browse firmware package contents |
| Flash | FlashWidget | Flash firmware to devices with progress |
| GPT | PartitionTableWidget | View and edit GPT partition tables |
| Workflow | WorkflowDesigner | Design and execute device workflows |
| Jobs | JobQueueWidget | Queue and manage device operations |
| Plugins | PluginManagerWidget | Manage installed plugins |
| Vendors | VendorManagerWidget | Manage vendor integrations |
| Transport | TransportMonitorWidget | Monitor USB/Serial/TCP statistics |
| Logs | LogViewerWidget | View and filter application logs |
| Diagnostics | DiagnosticsWidget | Run diagnostics, view system health |
| DevTools | DeveloperToolsWidget | Inspector, console, profiler |

### Settings

SettingsDialog - Application preferences (General, Appearance, Runtime, Transport, Plugins, Logging, Updates, Advanced).

## Workflows

### Discovery - Connect - Flash

1. Discovery: Scan for devices, select device, view details
2. Session: Connect to device, monitor status
3. Firmware: Load .mbp package, inspect contents
4. Flash: Select partitions, start flash, monitor progress

### Diagnostics

1. Open Diagnostics view
2. Click Run All to execute all checks
3. Review Runtime, Transport, Plugin, and Environment tabs
4. Export report to file

### Plugin Management

1. Open Plugin Manager view
2. Click Refresh to enumerate plugins
3. View plugin details in the Plugins tab
4. Check capabilities in the Capabilities tab

## Accessibility

MBoot Studio provides full accessibility support:

- All interactive widgets have accessibleName and accessibleDescription
- Tooltips on all buttons and controls
- Logical tab order for keyboard navigation
- Screen reader compatible labels
- High DPI support (Qt AA_EnableHighDpiScaling)
- Dark mode via ThemeManager

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+1..9, Ctrl+0 | Switch views |
| F5 | Discover devices |
| Ctrl+Q | Quit |
| Ctrl+comma | Settings |
| Esc | Close dialog |

## Platforms

| Platform | Compiler | Status |
|----------|----------|--------|
| Linux | GCC / Clang | Supported |
| Windows | MinGW-w64 | Supported |
| macOS | AppleClang | Supported |

## Testing

```bash
ctest --output-on-failure -j$(nproc)
```

18 test files covering:
- RuntimeBridge, RuntimeEventDispatcher, RuntimeErrorMapper
- RuntimeConverters, RuntimeModels, RuntimeTypes
- DeviceDiscoveryWidget, SessionWidget, FlashWidget
- PackageExplorer, DiagnosticsWidget, PluginManagerWidget
- All framework components

## Directory Structure

```
apps/mboot-studio/
  include/gui/
    runtime/          # RuntimeBridge, EventDispatcher, Models, etc.
    framework/        # MainWindow, ApplicationController, etc.
    discovery/        # DeviceDiscoveryWidget, DeviceListModel
    session/          # SessionWidget, SessionMonitor
    firmware/         # PackageExplorer, FirmwareInspector
    flash/            # FlashWidget, ProgressWindow, FlashStatistics
    gpt/              # PartitionTableWidget, GPTViewer
    workflow/          # WorkflowDesigner
    job/              # JobQueueWidget
    plugin/           # PluginManagerWidget
    vendor/           # VendorManagerWidget
    transport/        # TransportMonitorWidget
    logs/             # LogViewerWidget
    diagnostics/      # DiagnosticsWidget
    devtools/         # DeveloperToolsWidget
    settings/         # SettingsDialog
  src/                # Implementation files
  tests/              # GUI-specific tests
```

## Known Limitations

- Plugin enable/disable not available through public Runtime API
- Transport Monitor shows hardcoded statistics (not wired to Runtime)
- Workflow Designer and Job Queue are not wired to Runtime
- Vendor Manager details dialog shows placeholder text
- Settings dialog save/load are stubs (no persistence)

## Further Reading

- Architecture.md - Detailed architecture documentation
- Components.md - Component listing
- Models.md - Presentation models
- Theme.md - Theme system
- Shortcuts.md - Keyboard shortcuts
- Testing.md - Test suite
