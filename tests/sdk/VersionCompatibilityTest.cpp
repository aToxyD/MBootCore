#include <sdk/PluginCompatibility.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/PluginValidator.hpp>
#include <sdk/VendorRegistration.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace sdk = mbootcore::sdk;

TEST_CASE("VersionCompatibilityTest", "[sdk]") {
    SECTION("testDefaultVersionReport") {
        sdk::PluginManifest manifest;
        sdk::PluginCompatibility comp;
        auto report = comp.checkSDKVersion(manifest);
        REQUIRE(!report.currentSDKVersion.empty());
        REQUIRE(!report.minSupported.empty());
        REQUIRE(!report.maxSupported.empty());
    }

    SECTION("testSDKVersionConstants") {
        sdk::PluginManifest manifest;
        sdk::PluginCompatibility comp;
        auto report = comp.checkSDKVersion(manifest);
        REQUIRE(report.currentSDKVersion == std::string("1.0.0"));
        REQUIRE(report.minSupported == std::string("1.0.0"));
        REQUIRE(report.maxSupported == std::string("1.0.0"));
    }

    SECTION("testMinMaxVersion") {
        sdk::PluginManifest manifest;
        sdk::PluginCompatibility comp;
        auto report = comp.checkSDKVersion(manifest);
        REQUIRE(report.minSupported <= report.maxSupported);
    }

    SECTION("testValidVersionString") {
        sdk::PluginValidator validator;
        REQUIRE(validator.isValidVersion("1.0.0"));
        REQUIRE(validator.isValidVersion("2.0"));
        REQUIRE(validator.isValidVersion("1"));
    }

    SECTION("testInvalidVersionString") {
        sdk::PluginValidator validator;
        REQUIRE(!validator.isValidVersion("abc"));
        REQUIRE(!validator.isValidVersion("1.0.0-beta"));
        REQUIRE(!validator.isValidVersion(""));
    }

    SECTION("testPluginNameValidation") {
        sdk::PluginValidator validator;
        REQUIRE(validator.isValidName("MyPlugin"));
        REQUIRE(validator.isValidName("my-plugin"));
        REQUIRE(validator.isValidName("my.plugin"));
    }

    SECTION("testPluginNameWithSpecialChars") {
        sdk::PluginValidator validator;
        REQUIRE(!validator.isValidName("hello world"));
        REQUIRE(!validator.isValidName("foo/bar"));
    }

    SECTION("testEmptyNameInvalid") {
        sdk::PluginValidator validator;
        REQUIRE(!validator.isValidName(""));
    }

    SECTION("testManifestVersionCompatibility") {
        sdk::PluginManifest manifest;
        manifest.setPluginName("TestPlugin");
        manifest.setPluginVersion("1.0.0");
        manifest.setSDKVersion("1.0.0");

        sdk::PluginValidator validator;
        auto result = validator.validateManifest(manifest);
        REQUIRE(result.valid);
    }

    SECTION("testMultipleVersionChecks") {
        sdk::PluginManifest m1;
        m1.setPluginName("P1"); m1.setPluginVersion("1.0.0"); m1.setSDKVersion("1.0.0");
        sdk::PluginManifest m2;
        m2.setPluginName("P2"); m2.setPluginVersion("2.0.0"); m2.setSDKVersion("1.0.0");
        sdk::PluginManifest m3;
        m3.setPluginName("P3"); m3.setPluginVersion("999.0.0"); m3.setSDKVersion("1.0.0");

        sdk::PluginCompatibility comp;
        auto r1 = comp.checkAll(m1);
        auto r2 = comp.checkAll(m2);
        auto r3 = comp.checkAll(m3);
        REQUIRE(r1.compatible);
        REQUIRE(r2.compatible);
        REQUIRE(r3.compatible);
    }
}
