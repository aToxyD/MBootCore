#pragma once

#include <memory>
#include <string>
#include <mbootcore/diagnostics/DiagnosticsTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace diagnostics {

class DiagnosticsManager;

class DiagnosticSession {
public:
    explicit DiagnosticSession(DiagnosticsManager& manager);
    ~DiagnosticSession();

    DiagnosticSession(const DiagnosticSession&) = delete;
    DiagnosticSession& operator=(const DiagnosticSession&) = delete;
    DiagnosticSession(DiagnosticSession&&) noexcept;
    DiagnosticSession& operator=(DiagnosticSession&&) noexcept;

    Result<DiagnosticReport> start();
    Result<DiagnosticReport> runCheck(const std::string& checkId);
    Result<void> addIssue(const DiagnosticIssue& issue);
    Result<std::vector<DiagnosticIssue>> issues() const;
    Result<HealthStatus> healthStatus() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
