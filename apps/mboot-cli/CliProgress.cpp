#include "CliProgress.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace mboot {
namespace cli {

CliProgress::CliProgress() : m_enabled(true) {}
CliProgress::CliProgress(bool enabled) : m_enabled(enabled) {}

void CliProgress::start(const std::string& operation, uint64_t total) {
    m_operation = operation;
    m_total = total;
    m_current = 0;
    m_percent = 0.0;
    m_speed = 0.0;
    m_eta = 0;
    m_startTime = std::chrono::steady_clock::now();
    if (m_enabled) updateDisplay();
}

void CliProgress::update(uint64_t current) {
    m_current = current;
    if (m_total > 0) {
        m_percent = (static_cast<double>(m_current) / m_total) * 100.0;
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::steady_clock::now() - m_startTime).count();
        if (elapsed > 0.0) {
            m_speed = static_cast<double>(m_current) / elapsed;
            if (m_speed > 0.0 && m_total > m_current) {
                m_eta = static_cast<int>((m_total - m_current) / m_speed);
            }
        }
    }
    if (m_enabled) updateDisplay();
}

void CliProgress::setMessage(const std::string& msg) {
    m_message = msg;
    if (m_enabled) updateDisplay();
}

void CliProgress::finish(bool success) {
    if (m_enabled) {
        if (success) {
            std::cout << "\r" << m_operation << ": 100% - Complete."
                      << std::string(30, ' ') << std::endl;
        } else {
            std::cout << "\r" << m_operation << ": FAILED."
                      << std::string(30, ' ') << std::endl;
        }
    }
    std::cout.flush();
}

void CliProgress::updateDisplay() {
    std::ostringstream ss;
    ss << "\r" << m_operation << ": " << std::fixed << std::setprecision(1) << m_percent << "%"
       << " [" << m_current << "/" << m_total << " bytes";
    if (m_speed > 0.0) {
        ss << " @ " << std::fixed << std::setprecision(0) << m_speed << " B/s";
    }
    if (m_eta > 0) {
        ss << " ETA " << m_eta << "s";
    }
    if (!m_message.empty()) {
        ss << " | " << m_message;
    }
    ss << "]";
    std::cout << ss.str() << std::flush;
}

} // namespace cli
} // namespace mboot
