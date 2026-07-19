#include <sdk/APICompatibility.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;

TEST_CASE("APICompatibilityTest", "[sdk]") {
    SECTION("testDefaultConstruction") {
        sdk::APICompatibilityChecker checker;
        REQUIRE(true);
    }

    SECTION("testSDKVersionCompatible") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.checkSDKVersion("2.0.0", "1.0.0");
        REQUIRE(report.compatible);
        REQUIRE(report.breakingChanges.empty());
    }

    SECTION("testSDKVersionIncompatible") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.checkSDKVersion("1.0.0", "2.0.0");
        REQUIRE(!report.compatible);
        REQUIRE(!report.breakingChanges.empty());
    }

    SECTION("testBreakingChangeDetection") {
        sdk::APICompatibilityChecker checker;
        sdk::APIDifference diff;
        diff.className = "TestClass";
        diff.memberName = "testMethod";
        diff.type = sdk::APIDifference::ChangeType::Removed;

        REQUIRE(checker.isBreakingChange(diff));

        diff.type = sdk::APIDifference::ChangeType::Added;
        REQUIRE(!checker.isBreakingChange(diff));
    }

    SECTION("testNonBreakingChange") {
        sdk::APICompatibilityChecker checker;
        sdk::APIDifference diff;
        diff.className = "TestClass";
        diff.memberName = "newMethod";
        diff.type = sdk::APIDifference::ChangeType::Added;
        diff.breaking = false;

        REQUIRE(!checker.isBreakingChange(diff));

        diff.type = sdk::APIDifference::ChangeType::AccessChanged;
        REQUIRE(!checker.isBreakingChange(diff));
    }

    SECTION("testCompareVersions") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.compareVersions("old/path", "new/path");
        REQUIRE(!report.summary.empty());
    }

    SECTION("testGenerateReport") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.compareVersions("v1", "v2");
        std::string output = checker.generateReport(report, "1.0.0", "2.0.0");
        REQUIRE(!output.empty());
        REQUIRE(output.find("API Compatibility Report") != std::string::npos);
    }

    SECTION("testAddedCount") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.compareVersions("old", "new");
        REQUIRE(report.addedCount + report.removedCount + report.changedCount ==
                static_cast<int>(report.differences.size()));
    }

    SECTION("testRemovedCount") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.compareVersions("old", "new");
        REQUIRE(report.removedCount >= 0);
    }

    SECTION("testSummaryNotEmpty") {
        sdk::APICompatibilityChecker checker;
        auto report = checker.checkSDKVersion("1.0.0", "2.0.0");
        REQUIRE(!report.summary.empty());
    }
}
