# GUI Testing

## Test Structure

All tests are in `apps/mboot-studio/tests/`. Each component has a dedicated test file using Qt Test framework with `QTEST_MAIN`.

| Test File | Coverage |
|-----------|----------|
| `framework_test.cpp` | ThemeManager, SettingsManager, WindowStateManager, RecentFilesManager, ActionManager, ShortcutManager, CommandManager, NotificationCenter, SplashScreen, AboutDialog, UpdateChecker |
| `discovery_test.cpp` | DeviceListModel, DeviceTreeModel, DeviceFilterProxyModel, DeviceDetailsWidget, DevicePropertiesWidget |
| `session_test.cpp` | SessionWidget, SessionMonitor, SessionStatistics |
| `firmware_test.cpp` | PackageExplorer, FirmwareInspector, MetadataViewer, PartitionViewer, DependencyViewer, IntegrityChecker |
| `flash_test.cpp` | FlashWidget, FlashPlanViewer, PartitionSelection, ProgressWindow, FlashStatistics |
| `gpt_test.cpp` | PartitionTableWidget, GPTViewer, PartitionEditor, BackupDialog, RestoreDialog, CompareDialog |
| `workflow_test.cpp` | WorkflowDesigner, WorkflowGraph, StepInspector, RecoveryViewer |
| `job_test.cpp` | JobQueueWidget, JobHistoryWidget, SchedulerView, ProgressAggregatorWidget, RollbackViewer, RecoveryPolicyEditor |
| `plugin_test.cpp` | PluginManagerWidget |
| `vendor_test.cpp` | VendorManagerWidget |
| `transport_test.cpp` | TransportMonitorWidget |
| `logs_test.cpp` | LogViewerWidget |
| `settings_test.cpp` | SettingsDialog |
| `diagnostics_test.cpp` | DiagnosticsWidget |
| `devtools_test.cpp` | DeveloperToolsWidget |
| `theme_test.cpp` | ThemePreviewWidget |
| `l10n_test.cpp` | Locale and translator tests |
| `integration_test.cpp` | Cross-component integration tests |

## Running

```bash
cd build
ctest -R "^studio_" --output-on-failure
```

## Patterns

- **Widget creation** — verify widgets construct without crashing
- **Model tests** — verify data insertion, retrieval, and roles
- **Signal tests** — QSignalSpy to verify signal emission
- **Integration tests** — cross-component interaction verification

## Local Verification

Studio tests are registered in `apps/mboot-studio/tests/CMakeLists.txt` and run with all other project tests via `ctest`.
