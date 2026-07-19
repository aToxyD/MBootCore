#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mbootcore/diagnostics/DiagnosticsTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace diagnostics {

class DiagnosticsManager {
public:
    DiagnosticsManager();
    ~DiagnosticsManager();

    DiagnosticsManager(const DiagnosticsManager&) = delete;
    DiagnosticsManager& operator=(const DiagnosticsManager&) = delete;
    DiagnosticsManager(DiagnosticsManager&&) noexcept;
    DiagnosticsManager& operator=(DiagnosticsManager&&) noexcept;

    Result<DiagnosticReport> runFullDiagnostics();
    Result<DiagnosticReport> runCategory(DiagnosticCategory category);
    Result<DiagnosticReport> runCheck(const std::string& checkId);

    void registerCheck(std::unique_ptr<DiagnosticCheck> check);
    void unregisterCheck(const std::string& checkId);

    Result<std::vector<DiagnosticRecommendation>> getRecommendations(const DiagnosticReport& report);

    Result<void> setSessionId(const std::string& sessionId);
    std::string sessionId() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
