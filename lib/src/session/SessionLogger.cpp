#include "mbootcore/session/SessionLogger.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace mbootcore {
namespace session {

SessionLogger::SessionLogger()
    : m_sessionName("Default"), m_entries(10000) {}

SessionLogger::SessionLogger(const std::string& sessionName)
    : m_sessionName(sessionName), m_entries(10000) {}

SessionLogger::SessionLogger(SessionLogger&& other) noexcept
    : m_sessionName(std::move(other.m_sessionName))
    , m_maxEntries(other.m_maxEntries)
    , m_entries(std::move(other.m_entries))
    , m_head(other.m_head)
    , m_count(other.m_count) {}

SessionLogger& SessionLogger::operator=(SessionLogger&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessionName = std::move(other.m_sessionName);
        m_maxEntries = other.m_maxEntries;
        m_entries = std::move(other.m_entries);
        m_head = other.m_head;
        m_count = other.m_count;
    }
    return *this;
}

void SessionLogger::log(LogLevel level, const std::string& category, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_count < m_maxEntries) {
        m_entries[(m_head + m_count) % m_maxEntries] = {std::chrono::steady_clock::now(), level, message, category};
        ++m_count;
    } else {
        m_entries[m_head] = {std::chrono::steady_clock::now(), level, message, category};
        m_head = (m_head + 1) % m_maxEntries;
    }
}

void SessionLogger::debug(const std::string& category, const std::string& message) {
    log(LogLevel::Debug, category, message);
}

void SessionLogger::info(const std::string& category, const std::string& message) {
    log(LogLevel::Info, category, message);
}

void SessionLogger::warning(const std::string& category, const std::string& message) {
    log(LogLevel::Warning, category, message);
}

void SessionLogger::error(const std::string& category, const std::string& message) {
    log(LogLevel::Error, category, message);
}

void SessionLogger::critical(const std::string& category, const std::string& message) {
    log(LogLevel::Fatal, category, message);
}

void SessionLogger::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_head = 0;
    m_count = 0;
}

std::size_t SessionLogger::entryCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_count;
}

std::vector<LogEntry> SessionLogger::lastEntries(std::size_t count) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (count >= m_count) count = m_count;
    std::vector<LogEntry> result;
    result.reserve(count);
    size_t start = (m_head + m_count - count) % m_maxEntries;
    for (size_t i = 0; i < count; ++i) {
        result.push_back(m_entries[(start + i) % m_maxEntries]);
    }
    return result;
}

std::vector<LogEntry> SessionLogger::entriesByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LogEntry> result;
    for (size_t i = 0; i < m_count; ++i) {
        const auto& e = m_entries[(m_head + i) % m_maxEntries];
        if (e.category == category) result.push_back(e);
    }
    return result;
}

std::vector<LogEntry> SessionLogger::entriesByLevel(LogLevel level) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LogEntry> result;
    for (size_t i = 0; i < m_count; ++i) {
        const auto& e = m_entries[(m_head + i) % m_maxEntries];
        if (e.level == level) result.push_back(e);
    }
    return result;
}

std::string SessionLogger::exportText() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    for (size_t i = 0; i < m_count; ++i) {
        const auto& e = m_entries[(m_head + i) % m_maxEntries];
        oss << "[" << formatTimestamp(e.timestamp) << "] "
            << "[" << levelToString(e.level) << "] "
            << "[" << e.category << "] "
            << e.message << "\n";
    }
    return oss.str();
}

std::string SessionLogger::exportJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        nlohmann::json j;
        j["session"] = m_sessionName;
        nlohmann::json arr = nlohmann::json::array();
        for (size_t i = 0; i < m_count; ++i) {
            const auto& e = m_entries[(m_head + i) % m_maxEntries];
            nlohmann::json entry;
            entry["timestamp"] = formatTimestamp(e.timestamp);
            entry["level"] = levelToString(e.level);
            entry["category"] = e.category;
            entry["message"] = e.message;
            arr.push_back(std::move(entry));
        }
        j["entries"] = std::move(arr);
        return j.dump(2);
    } catch (...) {
        return "{}";
    }
}

std::string SessionLogger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        case LogLevel::None:    return "NONE";
    }
    return "UNKNOWN";
}

std::string SessionLogger::formatTimestamp(const std::chrono::steady_clock::time_point& tp) const {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    auto secs = ms / 1000;
    auto millis = ms % 1000;
    std::time_t t = static_cast<std::time_t>(secs);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << millis;
    return oss.str();
}

} // namespace session
} // namespace mbootcore
