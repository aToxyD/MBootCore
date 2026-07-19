#include <algorithm>
#include <sdk/SDKInfo.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace sdk = mbootcore::sdk;

TEST_CASE("SDKInfoTest", "[sdk]") {
    SECTION("testDefaultSDKInfo") {
        sdk::SDKInfo info;
        REQUIRE(info.versionInfo.sdkVersion.toString() == std::string("1.0.0"));
        REQUIRE(info.components.empty());
        REQUIRE(info.capabilities.empty());
        REQUIRE(info.supportedPlatforms.empty());
        REQUIRE(info.availableTransports.empty());
        REQUIRE(info.availableProtocols.empty());
        REQUIRE(info.availableFeatures.empty());
    }

    SECTION("testSDKInfoNotEmpty") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(!info.components.empty());
        REQUIRE(!info.capabilities.empty());
        REQUIRE(!info.supportedPlatforms.empty());
        REQUIRE(!info.availableTransports.empty());
        REQUIRE(!info.availableProtocols.empty());
        REQUIRE(!info.availableFeatures.empty());
    }

    SECTION("testSDKInfoComponents") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(info.components.size() >= 3);

        bool hasCore = false;
        bool hasSdk = false;
        for (const auto& c : info.components) {
            if (c.name == "mbootcore") hasCore = true;
            if (c.name == "mbootsdk") hasSdk = true;
            REQUIRE(!c.status.empty());
            REQUIRE(!c.version.empty());
        }
        REQUIRE(hasCore);
        REQUIRE(hasSdk);
    }

    SECTION("testSDKInfoCapabilities") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(info.capabilities.size() >= 5);

        bool hasDiscovery = false;
        bool hasFlashing = false;
        for (const auto& c : info.capabilities) {
            if (c.name == "device_discovery") hasDiscovery = true;
            if (c.name == "firmware_flashing") hasFlashing = true;
            REQUIRE(!c.description.empty());
        }
        REQUIRE(hasDiscovery);
        REQUIRE(hasFlashing);
    }

    SECTION("testSDKInfoToString") {
        auto info = sdk::SDKInfo::collect();
        auto str = info.toString();
        REQUIRE(!str.empty());
        REQUIRE(str.find("MBootCore SDK Information") != std::string::npos);
        REQUIRE(str.find("SDK Version:") != std::string::npos);
        REQUIRE(str.find("Components:") != std::string::npos);
        REQUIRE(str.find("Capabilities:") != std::string::npos);
    }

    SECTION("testSDKInfoToJson") {
        auto info = sdk::SDKInfo::collect();
        auto json = info.toJson();
        REQUIRE(!json.empty());
        REQUIRE(json.find("\"sdkVersion\"") != std::string::npos);
        REQUIRE(json.find("\"components\"") != std::string::npos);
        REQUIRE(json.find("\"capabilities\"") != std::string::npos);
        REQUIRE(json.find("\"supportedPlatforms\"") != std::string::npos);
    }

    SECTION("testSDKInfoCollect") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(info.versionInfo.sdkVersion.toString() == std::string("1.0.0"));
        REQUIRE(info.versionInfo.sdkVersion.major == 1u);
        REQUIRE(info.versionInfo.sdkVersion.minor == 0u);
        REQUIRE(info.versionInfo.sdkVersion.patch == 0u);
    }

    SECTION("testSupportedPlatforms") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(std::find(info.supportedPlatforms.begin(),
                          info.supportedPlatforms.end(), "Windows") != info.supportedPlatforms.end());
        REQUIRE(std::find(info.supportedPlatforms.begin(),
                          info.supportedPlatforms.end(), "Linux") != info.supportedPlatforms.end());
        REQUIRE(std::find(info.supportedPlatforms.begin(),
                          info.supportedPlatforms.end(), "macOS") != info.supportedPlatforms.end());
    }

    SECTION("testAvailableTransports") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(std::find(info.availableTransports.begin(),
                          info.availableTransports.end(), "USB") != info.availableTransports.end());
        REQUIRE(std::find(info.availableTransports.begin(),
                          info.availableTransports.end(), "Serial") != info.availableTransports.end());
        REQUIRE(std::find(info.availableTransports.begin(),
                          info.availableTransports.end(), "TCP") != info.availableTransports.end());
    }

    SECTION("testAvailableProtocols") {
        auto info = sdk::SDKInfo::collect();
        REQUIRE(std::find(info.availableProtocols.begin(),
                          info.availableProtocols.end(), "Sahara") != info.availableProtocols.end());
        REQUIRE(std::find(info.availableProtocols.begin(),
                          info.availableProtocols.end(), "Firehose") != info.availableProtocols.end());
    }
}
