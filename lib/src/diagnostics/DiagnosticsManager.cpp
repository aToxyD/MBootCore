#include <mbootcore/diagnostics/DiagnosticsManager.hpp>
#include <mbootcore/diagnostics/DiagnosticCheck.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>

namespace mbootcore { namespace diagnostics {

namespace {

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_MSC_VER) || defined(__MINGW32__)
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

HealthStatus worstHealth(const std::vector<DiagnosticIssue>& issues) {
    HealthStatus worst = HealthStatus::Healthy;
    for (const auto& issue : issues) {
        if (issue.status < worst) {
            worst = issue.status;
        }
    }
    return worst;
}

std::vector<DiagnosticRecommendation> generateRecommendations(const std::vector<DiagnosticIssue>& issues) {
    std::vector<DiagnosticRecommendation> recs;
    for (const auto& issue : issues) {
        DiagnosticRecommendation rec;
        rec.issueId = issue.id;
        if (issue.severity >= DiagnosticSeverity::Critical) {
            rec.action = "ImmediateAction";
            rec.description = "Immediate action required: " + issue.title;
            rec.priority = 1;
        } else if (issue.severity >= DiagnosticSeverity::Warning) {
            rec.action = "Investigate";
            rec.description = "Investigate and resolve: " + issue.title;
            rec.priority = 2;
        } else {
            rec.action = "Monitor";
            rec.description = "Monitor condition: " + issue.title;
            rec.priority = 3;
        }
        recs.push_back(rec);
    }
    return recs;
}

} // anonymous namespace

struct DiagnosticsManager::Impl {
    std::vector<std::unique_ptr<DiagnosticCheck>> checks;
    std::string sessionId;
};

DiagnosticsManager::DiagnosticsManager()
    : m_impl(std::make_unique<Impl>()) {}

DiagnosticsManager::~DiagnosticsManager() = default;

DiagnosticsManager::DiagnosticsManager(DiagnosticsManager&&) noexcept = default;
DiagnosticsManager& DiagnosticsManager::operator=(DiagnosticsManager&&) noexcept = default;

Result<DiagnosticReport> DiagnosticsManager::runFullDiagnostics() {
    DiagnosticReport report;
    report.timestamp = currentTimestamp();
    report.sessionId = m_impl->sessionId;

    for (const auto& check : m_impl->checks) {
        auto result = check->execute();
        if (result) {
            report.issues.push_back(result.value());
        } else {
            DiagnosticIssue err;
            err.id = check->id();
            err.title = check->name() + " failed to execute";
            err.description = "Check execution returned error";
            err.severity = DiagnosticSeverity::Error;
            err.status = HealthStatus::Critical;
            err.category = check->category();
            report.issues.push_back(err);
        }
    }

    report.overallHealth = worstHealth(report.issues);
    report.recommendations = generateRecommendations(report.issues);
    report.summary["totalChecks"] = std::to_string(report.issues.size());
    report.summary["overallHealth"] = std::to_string(static_cast<uint32_t>(report.overallHealth));

    return report;
}

Result<DiagnosticReport> DiagnosticsManager::runCategory(DiagnosticCategory category) {
    DiagnosticReport report;
    report.timestamp = currentTimestamp();
    report.sessionId = m_impl->sessionId;

    for (const auto& check : m_impl->checks) {
        if (check->category() == category) {
            auto result = check->execute();
            if (result) {
                report.issues.push_back(result.value());
            }
        }
    }

    report.overallHealth = worstHealth(report.issues);
    report.recommendations = generateRecommendations(report.issues);
    report.summary["totalChecks"] = std::to_string(report.issues.size());
    report.summary["category"] = std::to_string(static_cast<uint32_t>(category));
    report.summary["overallHealth"] = std::to_string(static_cast<uint32_t>(report.overallHealth));

    return report;
}

Result<DiagnosticReport> DiagnosticsManager::runCheck(const std::string& checkId) {
    for (const auto& check : m_impl->checks) {
        if (check->id() == checkId) {
            DiagnosticReport report;
            report.timestamp = currentTimestamp();
            report.sessionId = m_impl->sessionId;
            auto result = check->execute();
            if (result) {
                report.issues.push_back(result.value());
            }
            report.overallHealth = worstHealth(report.issues);
            report.recommendations = generateRecommendations(report.issues);
            report.summary["totalChecks"] = std::to_string(report.issues.size());
            report.summary["checkId"] = checkId;
            report.summary["overallHealth"] = std::to_string(static_cast<uint32_t>(report.overallHealth));
            return report;
        }
    }
    return ErrorCode::InvalidArgument;
}

void DiagnosticsManager::registerCheck(std::unique_ptr<DiagnosticCheck> check) {
    if (check) {
        m_impl->checks.push_back(std::move(check));
    }
}

void DiagnosticsManager::unregisterCheck(const std::string& checkId) {
    auto it = std::remove_if(m_impl->checks.begin(), m_impl->checks.end(),
        [&](const auto& c) { return c->id() == checkId; });
    m_impl->checks.erase(it, m_impl->checks.end());
}

Result<std::vector<DiagnosticRecommendation>> DiagnosticsManager::getRecommendations(const DiagnosticReport& report) {
    return report.recommendations;
}

Result<void> DiagnosticsManager::setSessionId(const std::string& sessionId) {
    m_impl->sessionId = sessionId;
    return {};
}

std::string DiagnosticsManager::sessionId() const {
    return m_impl->sessionId;
}

} }
