#include "mbootcore/logging/FileLogger.hpp"

#include <chrono>
#include <iomanip>
#include <ctime>

namespace mbootcore {

namespace {

std::tm safeGmtime(const std::time_t& tt) noexcept {
    std::tm result{};
#ifdef _WIN32
    gmtime_s(&result, &tt);
#else
    gmtime_r(&tt, &result);
#endif
    return result;
}

} // anonymous namespace

FileLogger::FileLogger(const std::string& filePath, LogLevel level)
    : m_level(level) {
    m_file.open(filePath, std::ios::out | std::ios::app);
}

FileLogger::~FileLogger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void FileLogger::log(LogLevel level, std::string_view tag,
                      const std::string& message) {
    if (level < m_level || !m_file.is_open()) return;

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    auto tm = safeGmtime(tt);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_file << "[" << std::put_time(&tm, "%H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count()
           << "] [" << levelPrefix(level) << "] [" << tag << "] " << message << '\n';
}

std::string_view FileLogger::levelPrefix(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "";
    }
}

} // namespace mbootcore
