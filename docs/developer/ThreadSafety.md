# Thread Safety

## Overview

MBootCore provides thread-safety guarantees for concurrent components used in multi-device sessions, background jobs, and asynchronous UI updates.

## Thread Safety Classification

| Classification | Meaning |
|---------------|---------|
| ✅ Thread-safe | All operations safe from any thread without external synchronization |
| ⚠️ Thread-compatible | Safe when different objects accessed from different threads |
| 🔒 Externally synchronized | Caller must hold a lock; documented in API |
| ❌ Not thread-safe | Single-threaded use only |

## Component Audit

### Runtime Engine

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `Runtime` | ✅ | Internal `std::mutex` on all public methods |
| `RuntimeSessionManager` | ✅ | Mutex-protected session map |
| `RuntimeEventBus` | ✅ | `std::mutex` + subscriber list pattern |
| `RuntimeConfig` | ✅ | `std::shared_mutex` (readers concurrent) |

### Workflow Engine

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `WorkflowEngine` | ✅ | Worker thread via `std::thread` |
| `WorkflowStep` | ⚠️ | Thread-compatible; each step on single thread |
| `WorkflowProgress` | ✅ | `std::mutex` + callback dispatch |
| `WorkflowRegistry` | ✅ | `std::mutex` on registration |

### Job Engine

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `JobScheduler` | ✅ | Thread pool with mutex-protected queue |
| `JobPipeline` | 🔒 | Externally synchronized; caller must serialize |
| `JobHistory` | ✅ | `std::mutex` on all mutations |
| `ProgressAggregator` | ✅ | Atomic counters, mutex for vectors |

### Session Manager

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `DeviceManager` | ✅ | `std::mutex` on session map |
| `DeviceSession` | 🔒 | State machine is single-threaded; observer dispatch uses callback list |
| `SessionLogger` | ✅ | `std::mutex` on all operations |
| `SessionStatistics` | ✅ | Atomic counters |

### Plugin System

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `PluginManager` | ✅ | `std::mutex` on plugin map |
| `PluginContext` | 🔒 | Caller must ensure context validity |

### Telemetry

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `TelemetryCollector` | ✅ | `std::mutex` on metric map; atomic counters |
| `StructuredLogger` | ✅ | `std::mutex` on output stream |

### Security

| Component | Thread Safety | Mechanism |
|-----------|--------------|-----------|
| `SecurityManager` | ✅ | `std::mutex` on internal state |

## Patterns

### Observer Pattern

Long-running operations communicate results back via callback lists or dispatch queues:

```cpp
// Register observer
auto handle = operation.onProgress([](uint64_t current, uint64_t total) {
    updateProgress(current, total);
});

// Worker thread notifies
operation.notifyProgress(current, total);
```

### Mutex Protection

Shared state is protected by mutexes:

```cpp
class SessionManager {
    mutable std::mutex mutex_;
    std::vector<SessionPtr> sessions_;
public:
    void addSession(SessionPtr s) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.push_back(std::move(s));
    }
};
```

### Atomic Counters

Performance-critical counters use atomics:

```cpp
class SessionStatistics {
    std::atomic<uint64_t> bytesWritten_{0};
    std::atomic<uint64_t> operations_{0};
};
```
