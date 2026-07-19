#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/telemetry/TelemetryTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace telemetry {

class TelemetryCollector {
public:
    TelemetryCollector();
    ~TelemetryCollector();
    TelemetryCollector(const TelemetryCollector&) = delete;
    TelemetryCollector& operator=(const TelemetryCollector&) = delete;
    TelemetryCollector(TelemetryCollector&&) noexcept;
    TelemetryCollector& operator=(TelemetryCollector&&) noexcept;

    Result<void> enable();
    Result<void> disable();
    Result<void> pause();
    Result<void> resume();
    TelemetryState state() const;

    Result<void> recordEvent(const TelemetryEvent& event);
    Result<void> recordMetric(const std::string& name, const std::string& value);
    Result<void> recordError(const std::string& component, const std::string& message);
    Result<void> recordCrash(const std::string& component, const std::string& stacktrace);

    Result<TelemetryReport> generateReport();
    Result<std::string> exportJson();
    Result<void> clear();

    void setSessionId(const std::string& id);
    std::string sessionId() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
