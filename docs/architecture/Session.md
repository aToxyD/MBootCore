# Session Engine

## Overview

The Session layer provides a unified public API for managing device sessions. It wraps the entire framework â€” discovery, protocol negotiation, pipeline, flash operations â€” behind a clean interface so callers never touch protocol internals.

## Architecture

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”گ
â”‚                       DeviceManager                           â”‚
â”‚  owns multiple DeviceSession objects, find by any attribute  â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”ک
                         â”‚
           â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”گ
           â”‚       DeviceSession          â”‚
           â”‚  Descriptor + FlashDevice   â”‚
           â”‚  + Pipeline + Logger        â”‚
           â”‚  + State + Stats + Obs      â”‚
           â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”ک
                      â”‚
         â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”گ
         â–¼            â–¼            â–¼
  DeviceDescriptor  IFlashDevice  BootPipeline
  (from discovery)  (from proto)  (from factory)
```

## Key Components

### SessionTypes

- `SessionState` â€” 15-state enum: Disconnected â†’ Discovered â†’ Negotiated â†’ Connecting â†’ Connected â†’ Initializing â†’ Ready â†’ Busy â†’ Reading â†’ Writing â†’ Erasing â†’ Resetting â†’ Finished â†’ Error â†’ Cancelled
- `SessionEvent` â€” 10 event types for state machine transitions
- `SessionStatistics` â€” bytes/ops/bps per read/write/erase with exponential moving average for throughput; tracks connection time, elapsed time, retries, failures
- `SessionOperation` â€” per-operation record (name, timestamps, bytes, success)
- `SessionConfig` â€” connect/operation/progress timeouts, maxRetries, maxRecoveryAttempts, enable flags for logging/history/statistics/auto-recovery

### SessionLogger

Thread-safe logging with configurable entry limits:

- Five levels: Debug, Info, Warning, Error, Critical
- Entries stored with timestamp, level, message, category
- Query methods: `lastEntries()`, `entriesByCategory()`, `entriesByLevel()`
- Export to text or JSON format
- Configurable max entries (10,000 default) with circular eviction
- Session name binding for contextual logging

### ISessionObserver

Seven lifecycle callbacks:

| Callback | Description |
|----------|-------------|
| `onStateChanged` | State transitions (oldState, newState) |
| `onProgress` | Progress updates (transferred, total) |
| `onError` | Error events |
| `onCompleted` | Session operation complete |
| `onDisconnected` | Device disconnected |
| `onOperationStarted` | Flash operation began |
| `onOperationFinished` | Flash operation completed |

### DeviceSession

Core session class that owns:

- `DeviceDescriptor` â€” device identification from discovery
- `IFlashDevice` â€” flash operations interface
- `BootPipeline` â€” boot stage orchestrator
- `SessionLogger` â€” session-specific logging
- `SessionStatistics` â€” performance metrics
- `SessionConfig` â€” configuration parameters
- `SessionState` â€” current state machine state
- `std::vector<ISessionObserver*>` â€” lifecycle observers

### DeviceSessionFactory

Creates `DeviceSession` instances from discovery descriptors:

```cpp
class DeviceSessionFactory {
    Result<std::unique_ptr<DeviceSession>>
    create(DeviceDescriptor descriptor, ProtocolRegistry& registry);
};
```

The factory:
1. Looks up the protocol factory in the registry matching the descriptor
2. Creates the appropriate `IFlashDevice` and `BootPipeline`
3. Wraps everything in a `DeviceSession`

### DeviceManager

Multi-session manager:

```cpp
class DeviceManager {
    Result<std::unique_ptr<DeviceSession>> createSession(DeviceDescriptor);
    Result<Unit> destroySession(std::string_view id);

    DeviceSession* findSession(std::string_view id);
    std::vector<DeviceSession*> findSessionsByVendor(Vendor vendor);
    std::vector<DeviceSession*> findSessionsByProtocol(ProtocolType protocol);
    std::vector<DeviceSession*> findSessionsByState(SessionState state);

    size_t sessionCount() const;
    void clear();
};
```

## Session Lifecycle

```
1. createSession(descriptor)   â†’ DeviceSession created (Disconnected)
2. session->connect()          â†’ State: Connected â†’ Initializing â†’ Ready
3. session->readMemory()       â†’ State: Busy â†’ Reading â†’ Ready
4. session->writeMemory()      â†’ State: Busy â†’ Writing â†’ Ready
5. session->eraseMemory()      â†’ State: Busy â†’ Erasing â†’ Ready
6. session->disconnect()       â†’ State: Disconnected
```

## Virtual Device Infrastructure

MBootCore includes virtual device implementations for testing without hardware:

**VirtualSaharaDevice** â€” simulates Sahara device behavior:
- Configurable version (V2/V3), chip ID, image size, chunk size
- NAK injection for error scenarios
- Timeout and disconnect simulation
- ProtocolTrace records all events for verification
- 10 pre-built test scenarios: handshake V2/V3, version mismatch, upload, NAK, reset, disconnect, timeout, unexpected packet

**VirtualFirehoseDevice** â€” simulates Firehose device behavior:
- Pre-queue ACK/NAK responses
- Configurable configure ACK with memory attributes
- Read ACK with raw_mode support
- Optional write capture for verifying program data
- Fault injection modes

**VirtualFlashDevice** â€” simulates a generic flash device:
- Configurable storage size, sector size, partition layout
- Supports read, write, erase operations
- Tracks operations for verification



