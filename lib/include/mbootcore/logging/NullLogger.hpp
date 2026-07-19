#pragma once

#include "mbootcore/domain/ILogger.hpp"

namespace mbootcore {

class NullLogger : public ILogger {
public:
    void log(LogLevel, std::string_view, const std::string&) override {}
    void setLevel(LogLevel) override {}
    LogLevel level() const noexcept override { return LogLevel::None; }
};

} // namespace mbootcore
