#pragma once

#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/session/SessionTypes.hpp>

#include <string>
#include <vector>
#include <chrono>
#include <mutex>

namespace mbootcore {
namespace session {

using LogLevel = mbootcore::LogLevel;

struct LogEntry {
    std::chrono::steady_clock::time_point timestamp;
    LogLevel level{LogLevel::Info};
    std::string message;
    std::string category;
};

class SessionLogger {
public:
    SessionLogger();
    explicit SessionLogger(const std::string& sessionName);
    ~SessionLogger() = default;

    SessionLogger(const SessionLogger&) = delete;
    SessionLogger& operator=(const SessionLogger&) = delete;
    SessionLogger(SessionLogger&& other) noexcept;
    SessionLogger& operator=(SessionLogger&& other) noexcept;

    void log(LogLevel level, const std::string& category, const std::string& message);
    void debug(const std::string& category, const std::string& message);
    void info(const std::string& category, const std::string& message);
    void warning(const std::string& category, const std::string& message);
    void error(const std::string& category, const std::string& message);
    void critical(const std::string& category, const std::string& message);

    void clear();
    std::size_t entryCount() const;
    std::vector<LogEntry> lastEntries(std::size_t count) const;
    std::vector<LogEntry> entriesByCategory(const std::string& category) const;
    std::vector<LogEntry> entriesByLevel(LogLevel level) const;

    std::string exportText() const;
    std::string exportJson() const;

    void setSessionName(const std::string& name) { m_sessionName = name; }
    std::string sessionName() const { return m_sessionName; }

    void setMaxEntries(std::size_t max) { m_maxEntries = max; }
    std::size_t maxEntries() const { return m_maxEntries; }

private:
    std::string m_sessionName;
    std::size_t m_maxEntries{10000};
    std::vector<LogEntry> m_entries;
    std::size_t m_head{0};
    std::size_t m_count{0};
    mutable std::mutex m_mutex;

    std::string levelToString(LogLevel level) const;
    std::string formatTimestamp(const std::chrono::steady_clock::time_point& tp) const;
    std::string escapeJson(const std::string& s) const;
};

} // namespace session
} // namespace mbootcore
