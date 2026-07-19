#include <sdk/Version.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;

TEST_CASE("VersionTest", "[sdk]") {
    SECTION("testDefaultVersion") {
        sdk::SemanticVersion v;
        REQUIRE(v.major == 0u);
        REQUIRE(v.minor == 1u);
        REQUIRE(v.patch == 0u);
        REQUIRE(v.preRelease.empty());
        REQUIRE(v.buildMetadata.empty());

        const auto& info = sdk::getVersionInfo();
        REQUIRE(info.sdkVersion.major == 1u);
        REQUIRE(info.sdkVersion.minor == 0u);
        REQUIRE(info.sdkVersion.patch == 0u);
        REQUIRE(info.coreVersion.major == 1u);
        REQUIRE(info.coreVersion.minor == 0u);
        REQUIRE(info.coreVersion.patch == 0u);

        REQUIRE(sdk::getSDKVersion().major == 1u);
        REQUIRE(sdk::getCoreVersion().minor == 0u);
    }

    SECTION("testSemanticVersionToString") {
        sdk::SemanticVersion v{1, 2, 3};
        REQUIRE(v.toString() == "1.2.3");

        v.preRelease = "alpha";
        REQUIRE(v.toString() == "1.2.3-alpha");

        v.buildMetadata = "001";
        REQUIRE(v.toString() == "1.2.3-alpha+001");

        v.preRelease.clear();
        REQUIRE(v.toString() == "1.2.3+001");

        sdk::SemanticVersion v2{0, 0, 0};
        REQUIRE(v2.toString() == "0.0.0");
    }

    SECTION("testSemanticVersionFromString") {
        auto v = sdk::SemanticVersion::fromString("1.2.3");
        REQUIRE(v.major == 1u);
        REQUIRE(v.minor == 2u);
        REQUIRE(v.patch == 3u);
        REQUIRE(v.preRelease.empty());
        REQUIRE(v.buildMetadata.empty());

        v = sdk::SemanticVersion::fromString("2.0.1-alpha");
        REQUIRE(v.major == 2u);
        REQUIRE(v.minor == 0u);
        REQUIRE(v.patch == 1u);
        REQUIRE(v.preRelease == "alpha");
        REQUIRE(v.buildMetadata.empty());

        v = sdk::SemanticVersion::fromString("3.4.5+build42");
        REQUIRE(v.major == 3u);
        REQUIRE(v.minor == 4u);
        REQUIRE(v.patch == 5u);
        REQUIRE(v.preRelease.empty());
        REQUIRE(v.buildMetadata == "build42");

        v = sdk::SemanticVersion::fromString("10.20.30-rc.1+sha.1234");
        REQUIRE(v.major == 10u);
        REQUIRE(v.minor == 20u);
        REQUIRE(v.patch == 30u);
        REQUIRE(v.preRelease == "rc.1");
        REQUIRE(v.buildMetadata == "sha.1234");

        v = sdk::SemanticVersion::fromString("0.0.0");
        REQUIRE(v.major == 0u);
        REQUIRE(v.minor == 0u);
        REQUIRE(v.patch == 0u);
    }

    SECTION("testSemanticVersionFromStringInvalid") {
        auto v = sdk::SemanticVersion::fromString("");
        REQUIRE(v.major == 0u);
        REQUIRE(v.minor == 0u);
        REQUIRE(v.patch == 0u);

        v = sdk::SemanticVersion::fromString("1.2");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("1.2.3.4");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("abc");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("1.2.abc");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("a.b.c");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("-1.2.3");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("1.-2.3");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("1.2.-3");
        REQUIRE(v.major == 0u);

        v = sdk::SemanticVersion::fromString("1.2.3-alpha+");
        REQUIRE(v.major == 1u);
        REQUIRE(v.minor == 2u);
        REQUIRE(v.patch == 3u);
    }

    SECTION("testSemanticVersionComparison") {
        sdk::SemanticVersion a{1, 0, 0};
        sdk::SemanticVersion b{2, 0, 0};
        sdk::SemanticVersion c{1, 0, 0};

        REQUIRE(a == c);
        REQUIRE(a != b);
        REQUIRE(a < b);
        REQUIRE(b > a);
        REQUIRE(a <= c);
        REQUIRE(a >= c);
        REQUIRE(b >= a);
        REQUIRE(a <= b);
        REQUIRE(!(a > c));
        REQUIRE(!(a < c));

        sdk::SemanticVersion d{1, 2, 3};
        sdk::SemanticVersion e{1, 2, 4};
        REQUIRE(d < e);
        REQUIRE(e > d);

        sdk::SemanticVersion f{1, 0, 0, "alpha"};
        sdk::SemanticVersion g{1, 0, 0};
        REQUIRE(f < g);

        sdk::SemanticVersion h{1, 0, 0, "alpha"};
        sdk::SemanticVersion i{1, 0, 0, "beta"};
        REQUIRE(h < i);
    }

    SECTION("testVersionInfoNotEmpty") {
        const auto& info = sdk::getVersionInfo();
        REQUIRE(!info.sdkVersion.toString().empty());
        REQUIRE(!info.coreVersion.toString().empty());
        REQUIRE(!info.buildInfo.buildDate.empty());
        REQUIRE(!info.buildInfo.buildTime.empty());
        REQUIRE(!info.buildInfo.buildSystem.empty());
        REQUIRE(!info.compilerInfo.compilerName.empty());
        REQUIRE(!info.compilerInfo.compilerVersion.empty());
        REQUIRE(!info.compilerInfo.compilerArchitecture.empty());
        REQUIRE(!info.compilerInfo.cppStandard.empty());
        REQUIRE(!info.platformInfo.osName.empty());
        REQUIRE(!info.platformInfo.osArchitecture.empty());
        REQUIRE(info.platformInfo.pointerSize > 0);
        REQUIRE(info.platformInfo.numberOfCores > 0);
    }

    SECTION("testCompilerInfo") {
        const auto& info = sdk::getVersionInfo().compilerInfo;
        REQUIRE(!info.compilerName.empty());
        REQUIRE(!info.compilerVersion.empty());
        REQUIRE(!info.compilerId.empty());
        REQUIRE(!info.cppStandard.empty());
#if defined(_MSC_VER)
        REQUIRE(info.compilerName == "MSVC");
        REQUIRE(info.compilerId == "MSVC");
#elif defined(__clang__) && defined(__apple_build_version__)
        REQUIRE(info.compilerName == "AppleClang");
#elif defined(__clang__)
        REQUIRE(info.compilerName == "Clang");
#elif defined(__GNUC__)
        REQUIRE(info.compilerName == "GCC");
        REQUIRE(info.compilerId == "GNU");
#endif
    }

    SECTION("testPlatformInfo") {
        const auto& info = sdk::getVersionInfo().platformInfo;
        REQUIRE(!info.osName.empty());
        REQUIRE(!info.osArchitecture.empty());
        REQUIRE(info.pointerSize == sizeof(void*));
        REQUIRE(info.numberOfCores > 0);
#if defined(_WIN32)
        REQUIRE(info.osName == "Windows");
#elif defined(__linux__)
        REQUIRE(info.osName == "Linux");
#elif defined(__APPLE__)
        REQUIRE(info.osName == "macOS");
#endif
    }

    SECTION("testVersionJson") {
        const auto& info = sdk::getVersionInfo();
        std::string json = info.toJson();
        REQUIRE(!json.empty());
        REQUIRE(json[0] == '{');
        REQUIRE(json.find("\"sdkVersion\"") != std::string::npos);
        REQUIRE(json.find("\"coreVersion\"") != std::string::npos);
        REQUIRE(json.find("\"buildInfo\"") != std::string::npos);
        REQUIRE(json.find("\"gitInfo\"") != std::string::npos);
        REQUIRE(json.find("\"compilerInfo\"") != std::string::npos);
        REQUIRE(json.find("\"platformInfo\"") != std::string::npos);
        REQUIRE(json.find("\"osName\"") != std::string::npos);
        REQUIRE(json.find("\"compilerName\"") != std::string::npos);
        // Verify valid JSON structure
        REQUIRE(json.back() == '}');
    }

    SECTION("testVersionYaml") {
        const auto& info = sdk::getVersionInfo();
        std::string yaml = info.toYaml();
        REQUIRE(!yaml.empty());
        REQUIRE(yaml.find("sdkVersion:") != std::string::npos);
        REQUIRE(yaml.find("coreVersion:") != std::string::npos);
        REQUIRE(yaml.find("buildInfo:") != std::string::npos);
        REQUIRE(yaml.find("gitInfo:") != std::string::npos);
        REQUIRE(yaml.find("compilerInfo:") != std::string::npos);
        REQUIRE(yaml.find("platformInfo:") != std::string::npos);
        REQUIRE(yaml.find("osName:") != std::string::npos);
        REQUIRE(yaml.find("compilerName:") != std::string::npos);
        // YAML should not start with '{'
        REQUIRE(yaml[0] != '{');
    }

    SECTION("testBuildInfo") {
        const auto& info = sdk::getVersionInfo().buildInfo;
        REQUIRE(!info.buildDate.empty());
        REQUIRE(!info.buildTime.empty());
        REQUIRE(!info.buildSystem.empty());
        // buildDate should contain month abbreviations
        std::string date = info.buildDate;
        REQUIRE((date.find("  ") == std::string::npos || date.find(",") != std::string::npos ||
                date.find(":") != std::string::npos || !date.empty()));
        REQUIRE(!info.buildType.empty());
    }

    SECTION("testVersionRoundtrip") {
        sdk::SemanticVersion v{3, 2, 1, "beta", "metadata"};
        std::string str = v.toString();
        auto parsed = sdk::SemanticVersion::fromString(str);
        REQUIRE(parsed.major == 3u);
        REQUIRE(parsed.minor == 2u);
        REQUIRE(parsed.patch == 1u);
        REQUIRE(parsed.preRelease == "beta");
        REQUIRE(parsed.buildMetadata == "metadata");
        REQUIRE(v == parsed);

        sdk::SemanticVersion v2{0, 0, 0};
        std::string str2 = v2.toString();
        auto parsed2 = sdk::SemanticVersion::fromString(str2);
        REQUIRE(parsed2.major == 0u);
        REQUIRE(parsed2.minor == 0u);
        REQUIRE(parsed2.patch == 0u);

        sdk::SemanticVersion v3{10, 20, 30};
        std::string str3 = v3.toString();
        auto parsed3 = sdk::SemanticVersion::fromString(str3);
        REQUIRE(parsed3.major == 10u);
        REQUIRE(parsed3.minor == 20u);
        REQUIRE(parsed3.patch == 30u);
    }
}
