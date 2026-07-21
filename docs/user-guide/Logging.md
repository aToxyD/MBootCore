# Logging System

## Overview

MBootCore provides a two-tier logging system: a classic logger interface (`ILogger`) with Console/File/Null implementations, and a StructuredLogger for JSON-line formatted output.

## Logger Interface

```cpp
#include <mbootcore/domain/ILogger.hpp>

class ILogger {
    virtual void log(LogLevel level, std::string_view message) = 0;
    virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0;
};
```

## Log Levels

| Level | Usage |
|-------|-------|
| Debug | Detailed diagnostic information |
| Info | Normal operational messages |
| Warning | Unexpected but recoverable conditions |
| Error | Operation failures |
| Critical | Unrecoverable failures |

## Logger Implementations

### ConsoleLogger

Writes formatted log messages to `stdout` with color-coded levels:

```cpp
#include <mbootcore/logging/ConsoleLogger.hpp>
auto logger = std::make_shared<ConsoleLogger>();
```

### FileLogger

Writes log messages to a file with automatic rotation:

```cpp
#include <mbootcore/logging/FileLogger.hpp>
auto logger = std::make_shared<FileLogger>("mbootcore.log");
```

### NullLogger

Discards all log messages (useful for testing or silent operation):

```cpp
#include <mbootcore/logging/NullLogger.hpp>
auto logger = std::make_shared<NullLogger>();
```

## StructuredLogger

The StructuredLogger emits JSON-line format output suitable for log aggregation systems:

```cpp
#include <mbootcore/logging/StructuredLogger.hpp>

mbootcore::StructuredLogger logger;
logger.log(mbootcore::LogLevel::Info, "Session", "Device connected");
```

### JSON Output Format

```json
{
    "timestamp": "2026-07-02T12:34:56.789Z",
    "severity": "INFO",
    "category": "Session",
    "thread": "0x1234",
    "caller": "DeviceSession::connect",
    "message": "Device connected"
}
```

Each log entry is a single JSON object followed by a newline (JSON Lines format).

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | ISO 8601 | UTC timestamp with millisecond precision |
| `severity` | string | Uppercase log level |
| `category` | string | Component or subsystem name |
| `thread` | string | Thread ID in hexadecimal |
| `caller` | string | Function name (if available) |
| `message` | string | Log message text |

## SessionLogger

`DeviceSession` uses a `SessionLogger` that provides thread-safe logging with configurable entry limits:

```cpp
#include <mbootcore/session/SessionLogger.hpp>

mbootcore::session::SessionLogger slogger("session-1");
slogger.log(mbootcore::LogLevel::Info, "Operation started");

// Query recent entries
auto recent = slogger.lastEntries(10);

// Export
auto json = slogger.exportToJson();
auto text = slogger.exportToText();

// Filter by level or category
auto warnings = slogger.entriesByLevel(mbootcore::LogLevel::Warning);
auto transportLogs = slogger.entriesByCategory("Transport");
```

- Default: 10,000 maximum entries with circular eviction
- Thread-safe via `std::mutex`
- Supports text and JSON export
- Session name binding for contextual logging
