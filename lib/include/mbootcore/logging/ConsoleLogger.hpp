#pragma once

#include "mbootcore/domain/ILogger.hpp"

namespace mbootcore {

class ConsoleLogger : public ILogger {
public:
    explicit ConsoleLogger(LogLevel level = LogLevel::Info);
    void log(LogLevel level, std::string_view tag,
             const std::string& message) override;
    void setLevel(LogLevel level) override { m_level = level; }
    LogLevel level() const noexcept override { return m_level; }

private:
    LogLevel m_level;
    static constexpr std::string_view levelPrefix(LogLevel level) noexcept;
};

} // namespace mbootcore
