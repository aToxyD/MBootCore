#include <catch2/catch_test_macros.hpp>

#include <sdk/Version.hpp>
#include <sdk/PluginCompatibility.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/SDKInfo.hpp>

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/DeviceTypes.hpp>
#include <mbootcore/domain/TransportTypes.hpp>

#include <string>

using namespace mbootcore;
using namespace mbootcore::sdk;

TEST_CASE("SDK namespace consistency", "[compatibility]") {
    SemanticVersion v;
    REQUIRE(v.major == 0u);
    REQUIRE(v.minor == 1u);
    REQUIRE(v.patch == 0u);

    VersionInfo vi;
    REQUIRE(vi.sdkVersion.major == 1u);
    REQUIRE(vi.sdkVersion.minor == 0u);
    REQUIRE(vi.sdkVersion.patch == 0u);

    const auto& sdkVer = getSDKVersion();
    REQUIRE(sdkVer.major == vi.sdkVersion.major);
    REQUIRE(sdkVer.minor == vi.sdkVersion.minor);
    REQUIRE(sdkVer.patch == vi.sdkVersion.patch);
}

TEST_CASE("Public headers are self-contained", "[compatibility]") {
    REQUIRE(static_cast<int>(ErrorCode::NotSupported) != 0);
    REQUIRE(static_cast<int>(discovery::BootMode::EDL) == 2);
    REQUIRE(static_cast<int>(transport::TransportType::USB) == 3);
    REQUIRE(static_cast<int>(vendor::VendorFamily::Qualcomm) == 1);
    REQUIRE(static_cast<int>(vendor::VendorFamily::MediaTek) == 2);
}

TEST_CASE("SDK version information consistency", "[compatibility]") {
    const auto& sdkVersion = getSDKVersion();
    REQUIRE(sdkVersion.toString() == std::string("1.0.0"));

    const auto& coreVersion = getCoreVersion();
    REQUIRE(coreVersion.toString().find('.') != std::string::npos);

    auto parsed = SemanticVersion::fromString("1.2.3");
    REQUIRE(parsed.major == 1u);
    REQUIRE(parsed.minor == 2u);
    REQUIRE(parsed.patch == 3u);
}

TEST_CASE("SemanticVersion comparison", "[compatibility]") {
    auto v1 = SemanticVersion::fromString("1.0.0");
    auto v2 = SemanticVersion::fromString("2.0.0");
    auto v3 = SemanticVersion::fromString("1.0.0");

    REQUIRE(v1 < v2);
    REQUIRE(v2 > v1);
    REQUIRE(v1 == v3);
    REQUIRE(v1 != v2);
    REQUIRE(v1 <= v3);
    REQUIRE(v2 >= v1);
}

TEST_CASE("Plugin compatibility version checks", "[compatibility]") {
    PluginCompatibility compat;

    PluginManifest manifest;
    manifest.setSDKVersion("1.0.0");

    auto report = compat.checkSDKVersion(manifest);
    REQUIRE(report.currentSDKVersion == std::string("1.0.0"));
    REQUIRE(report.minSupported == std::string("1.0.0"));
    REQUIRE(report.maxSupported == std::string("1.0.0"));
}

TEST_CASE("SDKDoctor basic checks", "[compatibility]") {
    SDKDoctor doctor;
    auto report = doctor.runAllChecks();

    REQUIRE(report.passedCount + report.warningCount + report.errorCount > 0);
    REQUIRE_FALSE(report.summary.empty());
}

TEST_CASE("Environment checker", "[compatibility]") {
    EnvironmentChecker checker;
    auto report = checker.check();

    REQUIRE_FALSE(report.osName.empty());
    REQUIRE_FALSE(report.architecture.empty());
}

TEST_CASE("Version constants self-consistency", "[compatibility]") {
    const auto& sdkVer = getSDKVersion();
    std::string sdkVerStr = sdkVer.toString();
    REQUIRE(sdkVerStr == std::string("1.0.0"));
}
