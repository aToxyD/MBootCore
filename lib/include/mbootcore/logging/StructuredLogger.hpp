#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/logging/LoggingTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace logging {

class StructuredLogger {
public:
    StructuredLogger();
    ~StructuredLogger();
    StructuredLogger(const StructuredLogger&) = delete;
    StructuredLogger& operator=(const StructuredLogger&) = delete;
    StructuredLogger(StructuredLogger&&) noexcept;
    StructuredLogger& operator=(StructuredLogger&&) noexcept;

    void log(LogLevel level, LogCategory category, const std::string& message,
             const std::string& scope = "", const std::string& file = "", uint32_t line = 0);

    void trace(const std::string& message, LogCategory cat = LogCategory::General);
    void debug(const std::string& message, LogCategory cat = LogCategory::General);
    void info(const std::string& message, LogCategory cat = LogCategory::General);
    void warning(const std::string& message, LogCategory cat = LogCategory::General);
    void error(const std::string& message, LogCategory cat = LogCategory::General);
    void fatal(const std::string& message, LogCategory cat = LogCategory::General);

    Result<void> setLevel(LogLevel level);
    LogLevel level() const;

    Result<void> setFormat(LogFormat format);
    LogFormat format() const;

    Result<void> setOutput(const std::string& filePath);
    Result<void> setRotation(const LogRotationConfig& config);

    void setSessionId(const std::string& id);
    void setDeviceId(const std::string& id);
    void setWorkflowId(const std::string& id);

    Result<std::vector<LogEntry>> entries(LogLevel minLevel = LogLevel::Trace) const;
    Result<std::vector<LogEntry>> entriesByCategory(LogCategory category) const;
    Result<std::vector<LogEntry>> entriesBySession(const std::string& sessionId) const;

    Result<void> addFilter(LogLevel minLevel, LogCategory category);
    Result<void> clearFilters();

    Result<void> flush();
    Result<void> clear();

    Result<std::string> exportJson() const;
    Result<std::string> exportText() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
