#include <catch2/catch_test_macros.hpp>
#include <mbootcore/diagnostics/DiagnosticsTypes.hpp>
#include <mbootcore/diagnostics/DiagnosticsManager.hpp>
#include <mbootcore/diagnostics/DiagnosticCheck.hpp>
#include <mbootcore/diagnostics/DiagnosticSession.hpp>

#include <memory>
#include <string>
#include <vector>
#include <map>

using namespace mbootcore;
using namespace mbootcore::diagnostics;

std::unique_ptr<DiagnosticsManager> createManagerWithChecks() {
    auto mgr = std::make_unique<DiagnosticsManager>();
    mgr->registerCheck(std::make_unique<RuntimeHealthCheck>());
    mgr->registerCheck(std::make_unique<MemoryHealthCheck>());
    mgr->registerCheck(std::make_unique<TransportHealthCheck>());
    mgr->registerCheck(std::make_unique<PipelineHealthCheck>());
    mgr->registerCheck(std::make_unique<PluginHealthCheck>());
    mgr->registerCheck(std::make_unique<DSPHealthCheck>());
    mgr->registerCheck(std::make_unique<ConfigurationHealthCheck>());
    mgr->registerCheck(std::make_unique<DeadlockDetectionCheck>());
    return mgr;
}

TEST_CASE("DiagnosticsTypesTest", "[diagnostics]") {
    SECTION("testHealthStatusEnumValues") {
        REQUIRE(static_cast<uint32_t>(HealthStatus::Healthy) == 0u);
        REQUIRE(static_cast<uint32_t>(HealthStatus::Warning) == 1u);
        REQUIRE(static_cast<uint32_t>(HealthStatus::Degraded) == 2u);
        REQUIRE(static_cast<uint32_t>(HealthStatus::Critical) == 3u);
        REQUIRE(static_cast<uint32_t>(HealthStatus::Unknown) == 4u);
    }

    SECTION("testDiagnosticSeverityEnumValues") {
        REQUIRE(static_cast<uint32_t>(DiagnosticSeverity::Info) == 0u);
        REQUIRE(static_cast<uint32_t>(DiagnosticSeverity::Warning) == 1u);
        REQUIRE(static_cast<uint32_t>(DiagnosticSeverity::Error) == 2u);
        REQUIRE(static_cast<uint32_t>(DiagnosticSeverity::Critical) == 3u);
        REQUIRE(static_cast<uint32_t>(DiagnosticSeverity::Fatal) == 4u);
    }

    SECTION("testDiagnosticCategoryEnumValues") {
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Runtime) == 0u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Memory) == 1u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Transport) == 2u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Pipeline) == 3u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Workflow) == 4u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::JobEngine) == 5u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Plugin) == 6u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::DSP) == 7u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Vendor) == 8u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Cache) == 9u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Performance) == 10u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Deadlock) == 11u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::ThreadHealth) == 12u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::ResourceLeak) == 13u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Configuration) == 14u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::System) == 15u);
        REQUIRE(static_cast<uint32_t>(DiagnosticCategory::Security) == 16u);
    }

    SECTION("testDiagnosticIssueDefaults") {
        DiagnosticIssue issue;
        REQUIRE(issue.id == std::string());
        REQUIRE(issue.title == std::string());
        REQUIRE(issue.description == std::string());
        REQUIRE(issue.severity == DiagnosticSeverity::Info);
        REQUIRE(issue.category == DiagnosticCategory::Runtime);
        REQUIRE(issue.status == HealthStatus::Healthy);
        REQUIRE(issue.details.empty());
    }

    SECTION("testDiagnosticIssueWithValues") {
        DiagnosticIssue issue;
        issue.id = "test-id";
        issue.title = "Test Title";
        issue.description = "Test description";
        issue.severity = DiagnosticSeverity::Warning;
        issue.category = DiagnosticCategory::Memory;
        issue.status = HealthStatus::Degraded;
        issue.details["key"] = "value";

        REQUIRE(issue.id == std::string("test-id"));
        REQUIRE(issue.title == std::string("Test Title"));
        REQUIRE(issue.description == std::string("Test description"));
        REQUIRE(issue.severity == DiagnosticSeverity::Warning);
        REQUIRE(issue.category == DiagnosticCategory::Memory);
        REQUIRE(issue.status == HealthStatus::Degraded);
        REQUIRE(issue.details["key"] == std::string("value"));
    }

    SECTION("testDiagnosticRecommendationDefaults") {
        DiagnosticRecommendation rec;
        REQUIRE(rec.issueId == std::string());
        REQUIRE(rec.action == std::string());
        REQUIRE(rec.description == std::string());
        REQUIRE(rec.priority == 0u);
    }

    SECTION("testDiagnosticReportDefaults") {
        DiagnosticReport report;
        REQUIRE(report.timestamp == std::string());
        REQUIRE(report.sessionId == std::string());
        REQUIRE(report.overallHealth == HealthStatus::Healthy);
        REQUIRE(report.issues.empty());
        REQUIRE(report.recommendations.empty());
        REQUIRE(report.summary.empty());
    }

    SECTION("testHealthStatusEntryDefaults") {
        HealthStatusEntry entry;
        REQUIRE(entry.component == std::string());
        REQUIRE(entry.status == HealthStatus::Healthy);
        REQUIRE(entry.message == std::string());
        REQUIRE(entry.metrics.empty());
    }
}

TEST_CASE("DiagnosticsManagerTest", "[diagnostics]") {
    SECTION("testRegisterCheck") {
        DiagnosticsManager mgr;
        REQUIRE(mgr.sessionId() == std::string());
        mgr.registerCheck(std::make_unique<RuntimeHealthCheck>());
        auto report = mgr.runFullDiagnostics();
        REQUIRE(report.isOk());
        REQUIRE(report.value().issues.size() == size_t(1));
    }

    SECTION("testUnregisterCheck") {
        auto mgr = createManagerWithChecks();
        mgr->unregisterCheck("check-runtime");
        auto report = mgr->runFullDiagnostics();
        REQUIRE(report.isOk());
        REQUIRE(report.value().issues.size() == size_t(7));
    }

    SECTION("testRunFullDiagnostics") {
        auto mgr = createManagerWithChecks();
        auto result = mgr->runFullDiagnostics();
        REQUIRE(result.isOk());
        auto& report = result.value();
        REQUIRE(report.issues.size() == size_t(8));
        REQUIRE(report.overallHealth == HealthStatus::Healthy);
        REQUIRE(!report.timestamp.empty());
        REQUIRE(report.recommendations.size() == size_t(8));
        REQUIRE(report.summary.find("totalChecks") != report.summary.end());
    }

    SECTION("testRunCategory") {
        auto mgr = createManagerWithChecks();
        auto result = mgr->runCategory(DiagnosticCategory::Memory);
        REQUIRE(result.isOk());
        auto& report = result.value();
        REQUIRE(report.issues.size() == size_t(1));
        REQUIRE(report.issues[0].id == std::string("check-memory"));
    }

    SECTION("testRunCategoryNoMatches") {
        DiagnosticsManager mgr;
        mgr.registerCheck(std::make_unique<RuntimeHealthCheck>());
        auto result = mgr.runCategory(DiagnosticCategory::Security);
        REQUIRE(result.isOk());
        REQUIRE(result.value().issues.empty());
    }

    SECTION("testRunSingleCheck") {
        auto mgr = createManagerWithChecks();
        auto result = mgr->runCheck("check-transport");
        REQUIRE(result.isOk());
        auto& report = result.value();
        REQUIRE(report.issues.size() == size_t(1));
        REQUIRE(report.issues[0].id == std::string("check-transport"));
    }

    SECTION("testRunSingleCheckNotFound") {
        auto mgr = createManagerWithChecks();
        auto result = mgr->runCheck("check-nonexistent");
        REQUIRE(result.isError());
    }

    SECTION("testGetRecommendations") {
        auto mgr = createManagerWithChecks();
        auto reportResult = mgr->runFullDiagnostics();
        REQUIRE(reportResult.isOk());
        auto& report = reportResult.value();
        auto recResult = mgr->getRecommendations(report);
        REQUIRE(recResult.isOk());
        REQUIRE(recResult.value().size() == size_t(8));
    }

    SECTION("testSessionId") {
        DiagnosticsManager mgr;
        auto setResult = mgr.setSessionId("session-001");
        REQUIRE(setResult.isOk());
        REQUIRE(mgr.sessionId() == std::string("session-001"));
        auto reportResult = mgr.runFullDiagnostics();
        REQUIRE(reportResult.isOk());
        REQUIRE(reportResult.value().sessionId == std::string("session-001"));
    }

    SECTION("testMoveSemantics") {
        DiagnosticsManager mgr1;
        mgr1.registerCheck(std::make_unique<RuntimeHealthCheck>());
        DiagnosticsManager mgr2 = std::move(mgr1);
        auto result = mgr2.runFullDiagnostics();
        REQUIRE(result.isOk());
        REQUIRE(result.value().issues.size() == size_t(1));
    }
}

TEST_CASE("DiagnosticCheckTest", "[diagnostics]") {
    SECTION("testRuntimeHealthCheck") {
        RuntimeHealthCheck check;
        REQUIRE(check.id() == std::string("check-runtime"));
        REQUIRE(check.name() == std::string("Runtime Health"));
        REQUIRE(check.category() == DiagnosticCategory::Runtime);
        auto result = check.execute();
        REQUIRE(result.isOk());
        auto& issue = result.value();
        REQUIRE(issue.status == HealthStatus::Healthy);
        REQUIRE(issue.severity == DiagnosticSeverity::Info);
    }

    SECTION("testMemoryHealthCheck") {
        MemoryHealthCheck check;
        REQUIRE(check.id() == std::string("check-memory"));
        REQUIRE(check.name() == std::string("Memory Health"));
        REQUIRE(check.category() == DiagnosticCategory::Memory);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testTransportHealthCheck") {
        TransportHealthCheck check;
        REQUIRE(check.id() == std::string("check-transport"));
        REQUIRE(check.name() == std::string("Transport Health"));
        REQUIRE(check.category() == DiagnosticCategory::Transport);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testPipelineHealthCheck") {
        PipelineHealthCheck check;
        REQUIRE(check.id() == std::string("check-pipeline"));
        REQUIRE(check.name() == std::string("Pipeline Health"));
        REQUIRE(check.category() == DiagnosticCategory::Pipeline);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testPluginHealthCheck") {
        PluginHealthCheck check;
        REQUIRE(check.id() == std::string("check-plugin"));
        REQUIRE(check.name() == std::string("Plugin Health"));
        REQUIRE(check.category() == DiagnosticCategory::Plugin);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testDSPHealthCheck") {
        DSPHealthCheck check;
        REQUIRE(check.id() == std::string("check-dsp"));
        REQUIRE(check.name() == std::string("DSP Health"));
        REQUIRE(check.category() == DiagnosticCategory::DSP);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testConfigurationHealthCheck") {
        ConfigurationHealthCheck check;
        REQUIRE(check.id() == std::string("check-config"));
        REQUIRE(check.name() == std::string("Configuration Health"));
        REQUIRE(check.category() == DiagnosticCategory::Configuration);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testDeadlockDetectionCheck") {
        DeadlockDetectionCheck check;
        REQUIRE(check.id() == std::string("check-deadlock"));
        REQUIRE(check.name() == std::string("Deadlock Detection"));
        REQUIRE(check.category() == DiagnosticCategory::Deadlock);
        auto result = check.execute();
        REQUIRE(result.isOk());
        REQUIRE(result.value().status == HealthStatus::Healthy);
    }

    SECTION("testAllChecksReturnInfoDetails") {
        RuntimeHealthCheck c1;
        MemoryHealthCheck c2;
        TransportHealthCheck c3;
        PipelineHealthCheck c4;
        PluginHealthCheck c5;
        DSPHealthCheck c6;
        ConfigurationHealthCheck c7;
        DeadlockDetectionCheck c8;

        auto r1 = c1.execute(); REQUIRE(!r1.value().details.empty());
        auto r2 = c2.execute(); REQUIRE(!r2.value().details.empty());
        auto r3 = c3.execute(); REQUIRE(!r3.value().details.empty());
        auto r4 = c4.execute(); REQUIRE(!r4.value().details.empty());
        auto r5 = c5.execute(); REQUIRE(!r5.value().details.empty());
        auto r6 = c6.execute(); REQUIRE(!r6.value().details.empty());
        auto r7 = c7.execute(); REQUIRE(!r7.value().details.empty());
        auto r8 = c8.execute(); REQUIRE(!r8.value().details.empty());
    }
}

TEST_CASE("DiagnosticSessionTest", "[diagnostics]") {
    SECTION("testSessionStart") {
        DiagnosticsManager mgr;
        mgr.registerCheck(std::make_unique<RuntimeHealthCheck>());
        DiagnosticSession session(mgr);
        auto result = session.start();
        REQUIRE(result.isOk());
        auto& report = result.value();
        REQUIRE(report.issues.size() == size_t(1));
        REQUIRE(report.overallHealth == HealthStatus::Healthy);
    }

    SECTION("testSessionRunCheck") {
        DiagnosticsManager mgr;
        mgr.registerCheck(std::make_unique<MemoryHealthCheck>());
        mgr.registerCheck(std::make_unique<TransportHealthCheck>());
        DiagnosticSession session(mgr);
        auto result = session.runCheck("check-memory");
        REQUIRE(result.isOk());
        REQUIRE(result.value().issues.size() == size_t(1));
        REQUIRE(result.value().issues[0].id == std::string("check-memory"));
    }

    SECTION("testSessionRunCheckNotFound") {
        DiagnosticsManager mgr;
        DiagnosticSession session(mgr);
        auto result = session.runCheck("check-nonexistent");
        REQUIRE(result.isError());
    }

    SECTION("testSessionAddIssue") {
        DiagnosticsManager mgr;
        DiagnosticSession session(mgr);
        DiagnosticIssue issue;
        issue.id = "manual-issue";
        issue.title = "Manual Issue";
        issue.severity = DiagnosticSeverity::Warning;
        issue.status = HealthStatus::Warning;
        auto addResult = session.addIssue(issue);
        REQUIRE(addResult.isOk());
        auto issuesResult = session.issues();
        REQUIRE(issuesResult.isOk());
        REQUIRE(issuesResult.value().size() == size_t(1));
        REQUIRE(issuesResult.value()[0].id == std::string("manual-issue"));
    }

    SECTION("testSessionHealthStatus") {
        DiagnosticsManager mgr;
        mgr.registerCheck(std::make_unique<RuntimeHealthCheck>());
        DiagnosticSession session(mgr);
        auto status = session.healthStatus();
        REQUIRE(status.isOk());
        REQUIRE(status.value() == HealthStatus::Healthy);
    }

    SECTION("testSessionHealthStatusAfterWarning") {
        DiagnosticsManager mgr;
        DiagnosticSession session(mgr);
        DiagnosticIssue issue;
        issue.id = "warn-issue";
        issue.title = "Warning";
        issue.status = HealthStatus::Warning;
        session.addIssue(issue);
        auto status = session.healthStatus();
        REQUIRE(status.isOk());
        REQUIRE(status.value() == HealthStatus::Warning);
    }

    SECTION("testSessionHealthStatusWorstWins") {
        DiagnosticsManager mgr;
        DiagnosticSession session(mgr);
        DiagnosticIssue healthy;
        healthy.id = "h";
        healthy.status = HealthStatus::Healthy;
        DiagnosticIssue critical;
        critical.id = "c";
        critical.status = HealthStatus::Critical;
        session.addIssue(healthy);
        session.addIssue(critical);
        auto status = session.healthStatus();
        REQUIRE(status.isOk());
        REQUIRE(status.value() == HealthStatus::Critical);
    }
}
