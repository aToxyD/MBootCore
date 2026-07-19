#include <sdk/SDKInfo.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace sdk = mbootcore::sdk;

TEST_CASE("EnvironmentCheckerTest", "[sdk]") {
    SECTION("testDefaultConstruction") {
        sdk::EnvironmentChecker checker;
        sdk::EnvironmentChecker::EnvironmentReport report;
        REQUIRE(report.valid == false);
        REQUIRE(report.osName.empty());
        REQUIRE(report.architecture.empty());
    }

    SECTION("testEnvironmentCheck") {
        sdk::EnvironmentChecker checker;
        auto report = checker.check();
        REQUIRE(!report.osName.empty());
        REQUIRE(!report.architecture.empty());
    }

    SECTION("testHasCMake") {
        sdk::EnvironmentChecker checker;
        bool hasCmake = checker.hasCMake();
        REQUIRE(true);
    }

    SECTION("testHasCompiler") {
        sdk::EnvironmentChecker checker;
        bool hasComp = checker.hasCompiler();
        REQUIRE(hasComp);
    }

    SECTION("testFindSDKPaths") {
        sdk::EnvironmentChecker checker;
        auto paths = checker.findSDKPaths();
        REQUIRE(!paths.empty());
    }

    SECTION("testEnvironmentReport") {
        sdk::EnvironmentChecker checker;
        auto report = checker.check();
        REQUIRE(!report.osName.empty());
        REQUIRE(report.availablePaths.size() > 0);
    }
}
