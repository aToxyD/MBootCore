# GUI Models

## DeviceListModel

Inherits `QAbstractListModel`. Used for flat device lists.

### Roles

| Role | Type | Description |
|------|------|-------------|
| `IdRole` | QString | Device unique identifier |
| `NameRole` | QString | Human-readable device name |
| `VendorRole` | QString | Vendor name |
| `ProtocolRole` | QString | Protocol name |
| `TransportRole` | QString | Transport type |
| `StatusRole` | QString | Connection status |
| `BootModeRole` | QString | Boot mode |
| `ConfidenceRole` | int | Detection confidence 0–100 |
| `ConnectedRole` | bool | Whether device is connected |
| `IconRole` | QIcon | Vendor-specific icon |

### Methods
`setDevices()`, `addDevice()`, `removeDevice()`, `updateDevice()`, `clear()`, `deviceAt(int)`, `deviceById(QString)`

## DeviceTreeModel

Inherits `QAbstractItemModel`. Groups devices by vendor → protocol → device:

```
Vendor (Qualcomm)
  ├── Protocol (Sahara)
  │   ├── Device 1
  │   └── Device 2
  └── Protocol (Firehose)
      └── Device 3
Vendor (MediaTek)
  └── Protocol (DA)
      └── Device 4
```

## DeviceFilterProxyModel

Inherits `QSortFilterProxyModel`. Filters by text search, vendor, protocol, connection status.

## PartitionSelection Model

Uses `QTableWidget` internally (checkable rows with partition name and size).

## Runtime Presentation Models

Lightweight GUI-side models in `RuntimeModels.hpp`, independent of Core types.

### Device & Session

| Model | Fields | Purpose |
|-------|--------|---------|
| `DeviceInfoView` | connectionPath, friendlyName, vendorName, protocolName, transportName, bootModeName, connectionStatus | Device information display |
| `DiscoveryResultView` | devices, scanDurationMs | Discovery scan results |
| `SessionInfoView` | sessionId, deviceName, isConnected, status, uptimeMs | Session status |
| `RuntimeStatusView` | state, initialized, connected, errorMessage | Runtime state |
| `TransportStatusView` | transportType, latencyMs, bandwidthBps, errorCount | Transport statistics |
| `ProgressInfoView` | percentage, message, elapsedMs, estimatedMs | Generic progress |

### Flash Workflow

| Model | Fields | Purpose |
|-------|--------|---------|
| `FlashOperationView` | status, packageName, imageCount, bytesTotal, bytesTransferred | Flash operation state |
| `FlashProgressView` | percentage, stageName, currentOperation, transferredBytes, speedBps, elapsedMs, etaMs | Flash progress |
| `FlashResultView` | success, errorMessage, totalBytesWritten, verificationPassed | Flash completion result |

### Diagnostics

| Model | Fields | Purpose |
|-------|--------|---------|
| `DiagnosticEntryView` | category, severity, message, timestamp, details | Single diagnostic entry |
| `DiagnosticReportView` | entries, totalChecks, passedCount, failedCount, warningCount | Full diagnostic report |
| `RuntimeHealthView` | activeSessions, activeWorkflows, queuedJobs, loadedPlugins, loadedVendors, connectedDevices, memoryUsageBytes, threadCount, transportState, uptimeSeconds | Runtime health snapshot |
| `DeviceHealthView` | deviceName, transportType, latencyMs, errorCount, connected | Per-device health |
| `RuntimeDiagnosticsView` | version, health, totalErrors, totalRecoveries, totalTimeouts, totalDisconnects, jobsExecuted, workflowsExecuted, averageFlashSpeedBps, averageReadSpeedBps, averageWriteSpeedBps, devices, osInfo, failures, warnings, recommendations | Full diagnostics view |

### Plugins & Capabilities

| Model | Fields | Purpose |
|-------|--------|---------|
| `PluginInfoView` | name, version, vendor, enabled, description | Plugin information |
| `PluginCapabilityView` | name, description, available, required | Plugin capability |
| `CapabilityView` | name, description, available, type | Runtime capability |
