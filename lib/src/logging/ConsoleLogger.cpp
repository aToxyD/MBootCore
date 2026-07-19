#include "mbootcore/logging/ConsoleLogger.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <mutex>

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

ConsoleLogger::ConsoleLogger(LogLevel level) : m_level(level) {}

void ConsoleLogger::log(LogLevel level, std::string_view tag,
                         const std::string& message) {
    if (level < m_level) return;

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    auto tm = safeGmtime(tt);

    std::cout << "[" << std::put_time(&tm, "%H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count()
              << "] [" << std::setw(7) << std::left << levelPrefix(level)
              << "] [" << tag << "] " << message << '\n';
}

constexpr std::string_view ConsoleLogger::levelPrefix(LogLevel level) noexcept {
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
