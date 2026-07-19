#pragma once

#include <cstdint>
#include <string_view>
#include <string>

namespace mbootcore {

enum class LogLevel : uint8_t {
    Trace   = 0,
    Debug   = 1,
    Info    = 2,
    Warning = 3,
    Error   = 4,
    Fatal   = 5,
    None    = 6
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, std::string_view tag,
                     const std::string& message) = 0;

    void debug(std::string_view tag, const std::string& msg) {
        log(LogLevel::Debug, tag, msg);
    }
    void info(std::string_view tag, const std::string& msg) {
        log(LogLevel::Info, tag, msg);
    }
    void warn(std::string_view tag, const std::string& msg) {
        log(LogLevel::Warning, tag, msg);
    }
    void error(std::string_view tag, const std::string& msg) {
        log(LogLevel::Error, tag, msg);
    }

    virtual void setLevel(LogLevel level) = 0;
    virtual LogLevel level() const noexcept = 0;
};

} // namespace mbootcore
