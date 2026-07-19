#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace mboot {
namespace cli {

class CliProgress {
public:
    CliProgress();
    explicit CliProgress(bool enabled);

    void start(const std::string& operation, uint64_t total = 0);
    void update(uint64_t current);
    void setMessage(const std::string& msg);
    void finish(bool success);
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    double percent() const { return m_percent; }
    uint64_t bytes() const { return m_current; }
    double speedBps() const { return m_speed; }
    int etaSeconds() const { return m_eta; }

private:
    void updateDisplay();

    bool m_enabled{true};
    std::string m_operation;
    uint64_t m_total{0};
    uint64_t m_current{0};
    double m_percent{0.0};
    double m_speed{0.0};
    int m_eta{0};
    std::string m_message;
    std::chrono::steady_clock::time_point m_startTime;
};

} // namespace cli
} // namespace mboot
