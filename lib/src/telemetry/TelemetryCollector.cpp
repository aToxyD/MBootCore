#include <mbootcore/telemetry/TelemetryCollector.hpp>

#include <nlohmann/json.hpp>

namespace mbootcore { namespace telemetry {

struct TelemetryCollector::Impl {
    TelemetryState m_state{ TelemetryState::Disabled };
    std::string m_sessionId;
    std::vector<TelemetryEvent> m_events;
};

TelemetryCollector::TelemetryCollector()
    : m_impl(std::make_unique<Impl>()) {}

TelemetryCollector::~TelemetryCollector() = default;
TelemetryCollector::TelemetryCollector(TelemetryCollector&&) noexcept = default;
TelemetryCollector& TelemetryCollector::operator=(TelemetryCollector&&) noexcept = default;

Result<void> TelemetryCollector::enable() {
    m_impl->m_state = TelemetryState::Enabled;
    return {};
}

Result<void> TelemetryCollector::disable() {
    m_impl->m_state = TelemetryState::Disabled;
    return {};
}

Result<void> TelemetryCollector::pause() {
    if (m_impl->m_state == TelemetryState::Enabled) {
        m_impl->m_state = TelemetryState::Paused;
    }
    return {};
}

Result<void> TelemetryCollector::resume() {
    if (m_impl->m_state == TelemetryState::Paused) {
        m_impl->m_state = TelemetryState::Enabled;
    }
    return {};
}

TelemetryState TelemetryCollector::state() const {
    return m_impl->m_state;
}

Result<void> TelemetryCollector::recordEvent(const TelemetryEvent& event) {
    if (m_impl->m_state != TelemetryState::Enabled) {
        return ErrorCode::InvalidState;
    }
    m_impl->m_events.push_back(event);
    return {};
}

Result<void> TelemetryCollector::recordMetric(const std::string& name, const std::string& value) {
    TelemetryEvent ev;
    ev.id = "metric_" + name;
    ev.category = TelemetryCategory::Statistics;
    ev.name = name;
    ev.data["value"] = value;
    ev.timestamp = std::chrono::system_clock::now();
    return recordEvent(ev);
}

Result<void> TelemetryCollector::recordError(const std::string& component, const std::string& message) {
    TelemetryEvent ev;
    ev.id = "error_" + component;
    ev.category = TelemetryCategory::Error;
    ev.name = "error";
    ev.data["component"] = component;
    ev.data["message"] = message;
    ev.timestamp = std::chrono::system_clock::now();
    return recordEvent(ev);
}

Result<void> TelemetryCollector::recordCrash(const std::string& component, const std::string& stacktrace) {
    TelemetryEvent ev;
    ev.id = "crash_" + component;
    ev.category = TelemetryCategory::Crash;
    ev.name = "crash";
    ev.data["component"] = component;
    ev.data["stacktrace"] = stacktrace;
    ev.timestamp = std::chrono::system_clock::now();
    return recordEvent(ev);
}

Result<TelemetryReport> TelemetryCollector::generateReport() {
    TelemetryReport report;
    report.sessionId = m_impl->m_sessionId;
    report.events = m_impl->m_events;
    report.generated = std::chrono::system_clock::now();

    for (const auto& e : m_impl->m_events) {
        report.eventCounts[e.name]++;
    }

    return report;
}

Result<std::string> TelemetryCollector::exportJson() {
    auto reportResult = generateReport();
    if (reportResult.isError()) {
        return reportResult.error();
    }

    auto& report = reportResult.value();
    try {
        nlohmann::json j;
        j["sessionId"] = report.sessionId;
        j["generated"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            report.generated.time_since_epoch()).count();

        nlohmann::json events = nlohmann::json::array();
        for (const auto& e : report.events) {
            nlohmann::json ev;
            ev["id"] = e.id;
            ev["category"] = static_cast<uint32_t>(e.category);
            ev["name"] = e.name;
            ev["data"] = nlohmann::json::object();
            for (const auto& [k, v] : e.data) {
                ev["data"][k] = v;
            }
            ev["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                e.timestamp.time_since_epoch()).count();
            events.push_back(std::move(ev));
        }
        j["events"] = std::move(events);

        nlohmann::json counts = nlohmann::json::object();
        for (const auto& [name, count] : report.eventCounts) {
            counts[name] = count;
        }
        j["eventCounts"] = std::move(counts);

        return j.dump(2);
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

Result<void> TelemetryCollector::clear() {
    m_impl->m_events.clear();
    return {};
}

void TelemetryCollector::setSessionId(const std::string& id) {
    m_impl->m_sessionId = id;
}

std::string TelemetryCollector::sessionId() const {
    return m_impl->m_sessionId;
}

} }
