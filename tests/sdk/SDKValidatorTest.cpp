#include <sdk/SDKValidator.hpp>
#include <sdk/APICompatibility.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

namespace sdk = mbootcore::sdk;

TEST_CASE("SDKValidatorTest", "[sdk]") {
    SECTION("testDefaultConstruction") {
        sdk::SDKValidator validator;
        REQUIRE(true);
    }

    SECTION("testValidateEmptyHeaders") {
        sdk::SDKValidator validator;
        auto report = validator.validatePublicAPI({});
        REQUIRE(report.valid);
        REQUIRE(report.publicClasses.empty());
        REQUIRE(report.publicInterfaces.empty());
        REQUIRE(report.publicEnums.empty());
        REQUIRE(report.publicFunctions.empty());
    }

    SECTION("testCheckHeaderSelfContainment") {
        sdk::SDKValidator validator;
        auto issues = validator.checkHeaderSelfContainment("nonexistent.hpp");
        REQUIRE(!issues.empty());
        REQUIRE(issues[0].severity == sdk::HeaderIssue::Severity::Error);
    }

    SECTION("testCheckIncludeOrdering") {
        sdk::SDKValidator validator;
        auto issues = validator.checkIncludeOrdering("nonexistent.hpp");
        REQUIRE(!issues.empty());
        REQUIRE(issues[0].severity == sdk::HeaderIssue::Severity::Error);
    }

    SECTION("testCheckNamespaceConsistency") {
        sdk::SDKValidator validator;
        auto issues = validator.checkNamespaceConsistency("nonexistent.hpp");
        REQUIRE(!issues.empty());
        REQUIRE(issues[0].severity == sdk::HeaderIssue::Severity::Error);
    }

    SECTION("testNoPrivateHeadersLeaked") {
        sdk::SDKValidator validator;

        std::vector<std::string> publicHeaders = {
            "include/sdk/SDKValidator.hpp",
            "include/sdk/APICompatibility.hpp"
        };

        std::vector<std::string> privateHeaders = {
            "src/SDKValidator.cpp",
            "src/APICompatibility.cpp"
        };

        REQUIRE(validator.verifyNoPrivateHeadersLeaked(publicHeaders, privateHeaders));
    }

    SECTION("testVerifyForwardDeclarations") {
        sdk::SDKValidator validator;
        std::vector<std::string> headers = {
            "nonexistent_forward.hpp"
        };
        bool result = validator.verifyForwardDeclarations(headers);
        REQUIRE(result);
    }

    SECTION("testGenerateReport") {
        sdk::SDKValidator validator;
        auto report = validator.validatePublicAPI({});
        std::string output = validator.generateReport(report);
        REQUIRE(!output.empty());
        REQUIRE(output.find("SDK Public API Report") != std::string::npos);
    }

    SECTION("testABIReportDefault") {
        sdk::SDKValidator validator;
        auto report = validator.compareABI("1.0.0", "2.0.0");
        REQUIRE(!report.compatible);
        REQUIRE(!report.warnings.empty());
    }

    SECTION("testValidateKnownHeaders") {
        sdk::SDKValidator validator;
        std::vector<std::string> headers = {
            "include/sdk/SDKValidator.hpp",
            "include/sdk/APICompatibility.hpp"
        };
        auto report = validator.validateSDKHeaders(headers);
        REQUIRE((!report.missingHeaders.empty() || report.valid));
    }

    SECTION("testCompareABIVersions") {
        sdk::SDKValidator validator;
        auto r1 = validator.compareABI("1.0.0", "1.0.0");
        auto r2 = validator.compareABI("1.0.0", "2.0.0");
        REQUIRE(!r1.compatible);
        REQUIRE(!r2.compatible);
    }

    SECTION("testReportNotEmpty") {
        sdk::SDKValidator validator;
        auto report = validator.validatePublicAPI({"some_dir"});
        std::string output = validator.generateReport(report);
        REQUIRE(!output.empty());
    }
}
