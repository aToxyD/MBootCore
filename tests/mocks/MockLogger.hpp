#pragma once

#include "mbootcore/domain/ILogger.hpp"

#include <vector>
#include <string>
#include <algorithm>

namespace mbootcore {

class MockLogger : public ILogger {
public:
    struct LogRecord {
        LogLevel level;
        std::string tag;
        std::string message;
    };

    void log(LogLevel level, std::string_view tag,
             const std::string& message) override {
        m_records.push_back({level, std::string(tag), message});
    }

    void setLevel(LogLevel level) override { m_level = level; }
    LogLevel level() const noexcept override { return m_level; }

    const std::vector<LogRecord>& records() const noexcept { return m_records; }
    void clear() { m_records.clear(); }

    bool hasTag(std::string_view tag) const noexcept {
        return std::any_of(m_records.begin(), m_records.end(),
            [tag](const LogRecord& r) { return r.tag == tag; });
    }

private:
    LogLevel m_level{LogLevel::Debug};
    std::vector<LogRecord> m_records;
};

} // namespace mbootcore
