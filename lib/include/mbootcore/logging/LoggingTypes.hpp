#pragma once

#include <mbootcore/domain/ILogger.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace mbootcore { namespace logging {

using LogLevel = mbootcore::LogLevel;

enum class LogFormat : uint32_t {
    Text = 0,
    JSON = 1
};

enum class LogCategory : uint32_t {
    General = 0,
    Transport = 1,
    Protocol = 2,
    Pipeline = 3,
    Workflow = 4,
    Job = 5,
    Plugin = 6,
    DSP = 7,
    Security = 8,
    Config = 9,
    Diagnostics = 10,
    Profiler = 11,
    Memory = 12,
    Session = 13,
    Device = 14
};

struct LogEntry {
    LogLevel level{ LogLevel::Info };
    LogCategory category{ LogCategory::General };
    std::string message;
    std::string scope;
    std::string threadId;
    std::string sessionId;
    std::string deviceId;
    std::string workflowId;
    std::chrono::system_clock::time_point timestamp;
    std::string fileName;
    uint32_t lineNumber{ 0 };
};

struct LogRotationConfig {
    bool enabled{ true };
    uint64_t maxSizeBytes{ 10 * 1024 * 1024 };
    uint32_t maxFiles{ 5 };
    bool compression{ false };
};

class StructuredLogger;

} }
