#include <mbootcore/logging/StructuredLogger.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

namespace mbootcore { namespace logging {

namespace {

constexpr size_t kRingBufferSize = 10000;

std::string levelToString(LogLevel lv) {
    switch (lv) {
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

std::string categoryToString(LogCategory cat) {
    switch (cat) {
        case LogCategory::General:       return "General";
        case LogCategory::Transport:     return "Transport";
        case LogCategory::Protocol:      return "Protocol";
        case LogCategory::Pipeline:      return "Pipeline";
        case LogCategory::Workflow:      return "Workflow";
        case LogCategory::Job:           return "Job";
        case LogCategory::Plugin:        return "Plugin";
        case LogCategory::DSP:           return "DSP";
        case LogCategory::Security:      return "Security";
        case LogCategory::Config:        return "Config";
        case LogCategory::Diagnostics:   return "Diagnostics";
        case LogCategory::Profiler:      return "Profiler";
        case LogCategory::Memory:        return "Memory";
        case LogCategory::Session:       return "Session";
        case LogCategory::Device:        return "Device";
    }
    return "Unknown";
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  tp.time_since_epoch()).count() % 1000;
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "."
       << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

struct FilterRule {
    LogLevel minLevel;
    LogCategory category;
};

}

struct StructuredLogger::Impl {
    LogLevel m_level{ LogLevel::Trace };
    LogFormat m_format{ LogFormat::Text };
    std::string m_filePath;
    std::ofstream m_fileStream;
    LogRotationConfig m_rotationConfig;
    std::vector<LogEntry> m_buffer;
    size_t m_writeIndex{ 0 };
    size_t m_count{ 0 };
    std::vector<FilterRule> m_filters;
    std::string m_sessionId;
    std::string m_deviceId;
    std::string m_workflowId;

    Impl() : m_buffer(kRingBufferSize) {}

    void appendEntry(LogEntry entry) {
        m_buffer[m_writeIndex] = std::move(entry);
        m_writeIndex = (m_writeIndex + 1) % kRingBufferSize;
        if (m_count < kRingBufferSize) ++m_count;
    }

    std::vector<LogEntry> allEntries() const {
        if (m_count == 0) return {};
        if (m_count < kRingBufferSize) {
            return std::vector<LogEntry>(m_buffer.begin(), m_buffer.begin() + static_cast<std::ptrdiff_t>(m_count));
        }
        std::vector<LogEntry> result;
        result.reserve(kRingBufferSize);
        for (size_t i = m_writeIndex; i < kRingBufferSize; ++i) {
            result.push_back(m_buffer[i]);
        }
        for (size_t i = 0; i < m_writeIndex; ++i) {
            result.push_back(m_buffer[i]);
        }
        return result;
    }

    bool passesFilters(LogLevel lv, LogCategory cat) const {
        if (!m_filters.empty()) {
            for (const auto& f : m_filters) {
                if (lv >= f.minLevel && cat == f.category) return true;
            }
            return false;
        }
        if (lv < m_level) return false;
        return true;
    }

    void flushToFile() {
        if (m_filePath.empty() || !m_fileStream.is_open()) return;

        auto entries = allEntries();
        for (const auto& e : entries) {
            if (m_format == LogFormat::JSON) {
                nlohmann::json entry;
                entry["timestamp"] = formatTimestamp(e.timestamp);
                entry["level"] = levelToString(e.level);
                entry["category"] = categoryToString(e.category);
                entry["message"] = e.message;
                entry["sessionId"] = e.sessionId;
                entry["deviceId"] = e.deviceId;
                entry["workflowId"] = e.workflowId;
                m_fileStream << entry.dump() << "\n";
            } else {
                m_fileStream << "[" << formatTimestamp(e.timestamp) << "] ["
                           << std::setfill(' ') << std::setw(7)
                           << levelToString(e.level) << "] ["
                           << categoryToString(e.category) << "] "
                           << e.message << "\n";
            }
        }
        m_fileStream.flush();
    }
};

StructuredLogger::StructuredLogger()
    : m_impl(std::make_unique<Impl>()) {}

StructuredLogger::~StructuredLogger() = default;
StructuredLogger::StructuredLogger(StructuredLogger&&) noexcept = default;
StructuredLogger& StructuredLogger::operator=(StructuredLogger&&) noexcept = default;

void StructuredLogger::log(LogLevel level, LogCategory category, const std::string& message,
                           const std::string& scope, const std::string& file, uint32_t line) {
    if (!m_impl->passesFilters(level, category)) return;

    LogEntry entry;
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.scope = scope;
    entry.fileName = file;
    entry.lineNumber = line;
    entry.timestamp = std::chrono::system_clock::now();

    {
        static thread_local std::string cachedThreadId = [] {
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            return ss.str();
        }();
        entry.threadId = cachedThreadId;
    }

    entry.sessionId = m_impl->m_sessionId;
    entry.deviceId = m_impl->m_deviceId;
    entry.workflowId = m_impl->m_workflowId;

    m_impl->appendEntry(std::move(entry));
}

void StructuredLogger::trace(const std::string& message, LogCategory cat) {
    log(LogLevel::Trace, cat, message);
}

void StructuredLogger::debug(const std::string& message, LogCategory cat) {
    log(LogLevel::Debug, cat, message);
}

void StructuredLogger::info(const std::string& message, LogCategory cat) {
    log(LogLevel::Info, cat, message);
}

void StructuredLogger::warning(const std::string& message, LogCategory cat) {
    log(LogLevel::Warning, cat, message);
}

void StructuredLogger::error(const std::string& message, LogCategory cat) {
    log(LogLevel::Error, cat, message);
}

void StructuredLogger::fatal(const std::string& message, LogCategory cat) {
    log(LogLevel::Fatal, cat, message);
}

Result<void> StructuredLogger::setLevel(LogLevel level) {
    m_impl->m_level = level;
    return {};
}

LogLevel StructuredLogger::level() const {
    return m_impl->m_level;
}

Result<void> StructuredLogger::setFormat(LogFormat format) {
    m_impl->m_format = format;
    return {};
}

LogFormat StructuredLogger::format() const {
    return m_impl->m_format;
}

Result<void> StructuredLogger::setOutput(const std::string& filePath) {
    if (m_impl->m_fileStream.is_open()) {
        m_impl->m_fileStream.close();
    }

    m_impl->m_filePath = filePath;
    m_impl->m_fileStream.open(filePath, std::ios::app);
    if (!m_impl->m_fileStream.is_open()) {
        return ErrorCode::InvalidArgument;
    }

    return {};
}

Result<void> StructuredLogger::setRotation(const LogRotationConfig& config) {
    m_impl->m_rotationConfig = config;
    return {};
}

void StructuredLogger::setSessionId(const std::string& id) {
    m_impl->m_sessionId = id;
}

void StructuredLogger::setDeviceId(const std::string& id) {
    m_impl->m_deviceId = id;
}

void StructuredLogger::setWorkflowId(const std::string& id) {
    m_impl->m_workflowId = id;
}

Result<std::vector<LogEntry>> StructuredLogger::entries(LogLevel minLevel) const {
    auto all = m_impl->allEntries();
    std::vector<LogEntry> result;
    std::copy_if(all.begin(), all.end(), std::back_inserter(result),
                 [minLevel](const LogEntry& e) { return e.level >= minLevel; });
    return result;
}

Result<std::vector<LogEntry>> StructuredLogger::entriesByCategory(LogCategory category) const {
    auto all = m_impl->allEntries();
    std::vector<LogEntry> result;
    std::copy_if(all.begin(), all.end(), std::back_inserter(result),
                 [category](const LogEntry& e) { return e.category == category; });
    return result;
}

Result<std::vector<LogEntry>> StructuredLogger::entriesBySession(const std::string& sessionId) const {
    auto all = m_impl->allEntries();
    std::vector<LogEntry> result;
    std::copy_if(all.begin(), all.end(), std::back_inserter(result),
                 [&sessionId](const LogEntry& e) { return e.sessionId == sessionId; });
    return result;
}

Result<void> StructuredLogger::addFilter(LogLevel minLevel, LogCategory category) {
    m_impl->m_filters.push_back({ minLevel, category });
    return {};
}

Result<void> StructuredLogger::clearFilters() {
    m_impl->m_filters.clear();
    return {};
}

Result<void> StructuredLogger::flush() {
    m_impl->flushToFile();
    return {};
}

Result<void> StructuredLogger::clear() {
    m_impl->m_buffer.assign(kRingBufferSize, LogEntry{});
    m_impl->m_writeIndex = 0;
    m_impl->m_count = 0;
    return {};
}

Result<std::string> StructuredLogger::exportJson() const {
    auto all = m_impl->allEntries();
    try {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& e : all) {
            nlohmann::json entry;
            entry["timestamp"] = formatTimestamp(e.timestamp);
            entry["level"] = levelToString(e.level);
            entry["category"] = categoryToString(e.category);
            entry["message"] = e.message;
            entry["sessionId"] = e.sessionId;
            entry["deviceId"] = e.deviceId;
            entry["workflowId"] = e.workflowId;
            entry["threadId"] = e.threadId;
            entry["fileName"] = e.fileName;
            entry["lineNumber"] = e.lineNumber;
            j.push_back(std::move(entry));
        }
        return j.dump(2);
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<std::string> StructuredLogger::exportText() const {
    auto all = m_impl->allEntries();
    std::ostringstream ss;
    for (const auto& e : all) {
        ss << "[" << formatTimestamp(e.timestamp) << "] ["
           << std::setfill(' ') << std::setw(7)
           << levelToString(e.level) << "] ["
           << categoryToString(e.category) << "] "
           << e.message;

        if (!e.scope.empty()) {
            ss << " {" << e.scope << "}";
        }
        if (!e.fileName.empty()) {
            ss << " (" << e.fileName << ":" << e.lineNumber << ")";
        }

        ss << "\n";
    }
    return ss.str();
}

} }
