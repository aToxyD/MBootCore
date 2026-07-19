#include <sdk/SDKInfo.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace sdk = mbootcore::sdk;

TEST_CASE("SDKDoctorTest", "[sdk]") {
    SECTION("testDefaultConstruction") {
        sdk::SDKDoctor doctor;
        sdk::SDKDoctor::DoctorReport report;
        REQUIRE(report.allPassed == false);
        REQUIRE(report.results.empty());
        REQUIRE(report.passedCount == 0);
        REQUIRE(report.warningCount == 0);
        REQUIRE(report.errorCount == 0);
        REQUIRE(report.summary.empty());
    }

    SECTION("testRunAllChecks") {
        sdk::SDKDoctor doctor;
        auto report = doctor.runAllChecks();
        REQUIRE(!report.results.empty());
        REQUIRE(report.results.size() >= 6);
        REQUIRE(!report.summary.empty());
    }

    SECTION("testCheckInstallation") {
        sdk::SDKDoctor doctor;
        auto result = doctor.checkInstallation();
        REQUIRE(!result.check.empty());
        REQUIRE(!result.message.empty());
        REQUIRE(!result.details.empty());
    }

    SECTION("testCheckCompiler") {
        sdk::SDKDoctor doctor;
        auto result = doctor.checkCompiler();
        REQUIRE(!result.check.empty());
        REQUIRE(!result.message.empty());
    }

    SECTION("testCheckDependencies") {
        sdk::SDKDoctor doctor;
        auto result = doctor.checkDependencies();
        REQUIRE(!result.check.empty());
        REQUIRE(!result.message.empty());
        REQUIRE(!result.details.empty());
    }

    SECTION("testGenerateReport") {
        sdk::SDKDoctor doctor;
        auto report = doctor.runAllChecks();
        auto str = doctor.generateReport(report);
        REQUIRE(!str.empty());
        REQUIRE(str.find("MBootCore SDK Doctor Report") != std::string::npos);
        REQUIRE(str.find("Summary:") != std::string::npos);
        REQUIRE(str.find("Overall:") != std::string::npos);
    }

    SECTION("testDoctorReportNotEmpty") {
        sdk::SDKDoctor doctor;
        auto report = doctor.runAllChecks();
        REQUIRE((report.passedCount > 0 || report.warningCount > 0 || report.errorCount > 0));
        REQUIRE(report.results.size() > 0);
    }

    SECTION("testResultsAggregation") {
        sdk::SDKDoctor doctor;
        auto report = doctor.runAllChecks();

        int total = report.passedCount + report.warningCount + report.errorCount;
        REQUIRE(total == (int)report.results.size());

        REQUIRE(report.allPassed == (report.errorCount == 0));
    }
}
