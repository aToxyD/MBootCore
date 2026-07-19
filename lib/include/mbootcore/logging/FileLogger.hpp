#pragma once

#include "mbootcore/domain/ILogger.hpp"

#include <fstream>
#include <mutex>

namespace mbootcore {

class FileLogger final : public ILogger {
public:
    explicit FileLogger(const std::string& filePath,
                        LogLevel level = LogLevel::Debug);
    ~FileLogger() override;

    void log(LogLevel level, std::string_view tag,
             const std::string& message) override;
    void setLevel(LogLevel level) noexcept override { m_level = level; }
    LogLevel level() const noexcept override { return m_level; }

private:
    static std::string_view levelPrefix(LogLevel level) noexcept;

    std::ofstream m_file;
    LogLevel m_level;
    std::mutex m_mutex;
};

} // namespace mbootcore
