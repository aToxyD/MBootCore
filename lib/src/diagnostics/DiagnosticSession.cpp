#include <mbootcore/diagnostics/DiagnosticSession.hpp>
#include <mbootcore/diagnostics/DiagnosticsManager.hpp>

#include <algorithm>

namespace mbootcore { namespace diagnostics {

namespace {

HealthStatus worstHealthFromIssues(const std::vector<DiagnosticIssue>& issues) {
    HealthStatus worst = HealthStatus::Healthy;
    for (const auto& issue : issues) {
        if (issue.status > worst) {
            worst = issue.status;
        }
    }
    return worst;
}

} // anonymous namespace

struct DiagnosticSession::Impl {
    DiagnosticsManager* manager;
    std::vector<DiagnosticIssue> issues;
};

DiagnosticSession::DiagnosticSession(DiagnosticsManager& manager)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->manager = &manager;
}

DiagnosticSession::~DiagnosticSession() = default;

DiagnosticSession::DiagnosticSession(DiagnosticSession&&) noexcept = default;
DiagnosticSession& DiagnosticSession::operator=(DiagnosticSession&&) noexcept = default;

Result<DiagnosticReport> DiagnosticSession::start() {
    auto report = m_impl->manager->runFullDiagnostics();
    if (report) {
        m_impl->issues = report.value().issues;
    }
    return report;
}

Result<DiagnosticReport> DiagnosticSession::runCheck(const std::string& checkId) {
    return m_impl->manager->runCheck(checkId);
}

Result<void> DiagnosticSession::addIssue(const DiagnosticIssue& issue) {
    m_impl->issues.push_back(issue);
    return {};
}

Result<std::vector<DiagnosticIssue>> DiagnosticSession::issues() const {
    return m_impl->issues;
}

Result<HealthStatus> DiagnosticSession::healthStatus() const {
    return worstHealthFromIssues(m_impl->issues);
}

} }
