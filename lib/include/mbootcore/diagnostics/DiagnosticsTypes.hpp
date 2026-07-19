#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace diagnostics {

enum class HealthStatus : uint32_t { Healthy=0, Warning=1, Degraded=2, Critical=3, Unknown=4 };
enum class DiagnosticSeverity : uint32_t { Info=0, Warning=1, Error=2, Critical=3, Fatal=4 };
enum class DiagnosticCategory : uint32_t {
    Runtime=0, Memory=1, Transport=2, Pipeline=3, Workflow=4,
    JobEngine=5, Plugin=6, DSP=7, Vendor=8, Cache=9,
    Performance=10, Deadlock=11, ThreadHealth=12, ResourceLeak=13,
    Configuration=14, System=15, Security=16
};

struct DiagnosticIssue {
    std::string id;
    std::string title;
    std::string description;
    DiagnosticSeverity severity{DiagnosticSeverity::Info};
    DiagnosticCategory category{DiagnosticCategory::Runtime};
    HealthStatus status{HealthStatus::Healthy};
    std::map<std::string,std::string> details;
};

struct DiagnosticRecommendation {
    std::string issueId;
    std::string action;
    std::string description;
    uint32_t priority{0};
};

struct DiagnosticReport {
    std::string timestamp;
    std::string sessionId;
    HealthStatus overallHealth{HealthStatus::Healthy};
    std::vector<DiagnosticIssue> issues;
    std::vector<DiagnosticRecommendation> recommendations;
    std::map<std::string,std::string> summary;
};

struct HealthStatusEntry {
    std::string component;
    HealthStatus status{HealthStatus::Healthy};
    std::string message;
    std::map<std::string,std::string> metrics;
};

class DiagnosticsManager;
class DiagnosticSession;
class DiagnosticCheck;

} }
