#include <sdk/PluginManifest.hpp>
#include <sdk/PluginValidator.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;

TEST_CASE("ManifestValidationTest", "[sdk]") {
    SECTION("testDefaultManifestValid") {
        sdk::PluginManifest manifest;
        REQUIRE(manifest.isValid());
        REQUIRE(manifest.pluginName() == std::string("unnamed"));
        REQUIRE(manifest.pluginVersion() == std::string("1.0.0"));
        REQUIRE(manifest.sdkVersion() == std::string("1.0.0"));
    }

    SECTION("testManifestWithName") {
        sdk::PluginManifest manifest;
        manifest.setPluginName("TestPlugin");
        REQUIRE(manifest.pluginName() == std::string("TestPlugin"));
    }

    SECTION("testManifestWithVersion") {
        sdk::PluginManifest manifest;
        manifest.setPluginVersion("1.0.0");
        manifest.setSDKVersion("2.0.0");
        REQUIRE(manifest.pluginVersion() == std::string("1.0.0"));
        REQUIRE(manifest.sdkVersion() == std::string("2.0.0"));
    }

    SECTION("testManifestToJson") {
        sdk::PluginManifest manifest;
        manifest.setPluginName("JsonPlugin");
        manifest.setPluginVersion("1.0.0");
        manifest.setSDKVersion("2.0.0");
        manifest.setVendor("TestVendor");

        std::string json = manifest.toJson();
        REQUIRE(!json.empty());
        REQUIRE(json.find("JsonPlugin") != std::string::npos);
    }

    SECTION("testManifestFromJson") {
        std::string json = R"({
            "pluginName": "FromJsonPlugin",
            "pluginVersion": "2.0.0",
            "sdkVersion": "2.0.0"
        })";

        auto manifest = sdk::PluginManifest::fromJson(json);
        REQUIRE(manifest.pluginName() == std::string("FromJsonPlugin"));
    }

    SECTION("testManifestJsonRoundtrip") {
        sdk::PluginManifest original;
        original.setPluginName("RoundtripPlugin");
        original.setPluginVersion("3.0.0");
        original.setSDKVersion("2.0.0");
        original.setVendor("RoundtripVendor");
        original.setDescription("A roundtrip test manifest");

        std::string json = original.toJson();
        REQUIRE(!json.empty());

        auto restored = sdk::PluginManifest::fromJson(json);
        REQUIRE(restored.pluginName() == original.pluginName());
        REQUIRE(restored.pluginVersion() == original.pluginVersion());
        REQUIRE(restored.sdkVersion() == original.sdkVersion());
        REQUIRE(restored.vendor() == original.vendor());
        REQUIRE(restored.description() == original.description());
    }

    SECTION("testManifestValidationErrors") {
        sdk::PluginManifest manifest;
        manifest.setPluginName("");
        manifest.setPluginVersion("");
        manifest.setSDKVersion("");

        auto errors = manifest.validate();
        REQUIRE(errors.size() >= 2);
    }

    SECTION("testValidateVendor") {
        sdk::PluginValidator validator;
        sdk::VendorRegistration reg;
        reg.name = "ValidVendor";
        reg.displayName = "Valid Vendor Inc.";

        auto result = validator.validateVendor(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateProtocol") {
        sdk::PluginValidator validator;
        sdk::ProtocolRegistration reg;
        reg.name = "ValidProtocol";
        reg.version = "1.0.0";
        reg.protocolType = mbootcore::discovery::ProtocolType::Sahara;

        auto result = validator.validateProtocol(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateTransport") {
        sdk::PluginValidator validator;
        sdk::TransportRegistration reg;
        reg.name = "ValidTransport";
        reg.version = "1.0.0";

        auto result = validator.validateTransport(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateWorkflow") {
        sdk::PluginValidator validator;
        sdk::WorkflowRegistration reg;
        reg.name = "ValidWorkflow";
        reg.version = "1.0.0";

        auto result = validator.validateWorkflow(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateJob") {
        sdk::PluginValidator validator;
        sdk::JobRegistration reg;
        reg.name = "ValidJob";
        reg.jobType = "flash";

        auto result = validator.validateJob(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidatePackage") {
        sdk::PluginValidator validator;
        sdk::PackageRegistration reg;
        reg.name = "ValidPackage";
        reg.version = "1.0.0";

        auto result = validator.validatePackage(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateDiscovery") {
        sdk::PluginValidator validator;
        sdk::DiscoveryRegistration reg;
        reg.name = "ValidDiscovery";
        reg.version = "1.0.0";

        auto result = validator.validateDiscovery(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateCapability") {
        sdk::PluginValidator validator;
        sdk::CapabilityRegistration reg;
        reg.name = "ValidCapability";
        reg.version = "1.0.0";
        reg.capability = mbootcore::plugin::PluginCapability::Protocol;

        auto result = validator.validateCapability(reg);
        REQUIRE(result.valid);
    }

    SECTION("testValidateManifest") {
        sdk::PluginValidator validator;
        sdk::PluginManifest manifest;
        manifest.setPluginName("ManifestPlugin");
        manifest.setPluginVersion("1.0.0");
        manifest.setSDKVersion("2.0.0");

        auto result = validator.validateManifest(manifest);
        REQUIRE(result.valid);
    }

    SECTION("testValidateAllComponents") {
        sdk::PluginValidator validator;
        sdk::PluginManifest manifest;
        manifest.setPluginName("AllPlugin");
        manifest.setPluginVersion("1.0.0");
        manifest.setSDKVersion("2.0.0");

        std::vector<sdk::VendorRegistration> vendors;
        sdk::VendorRegistration vreg;
        vreg.name = "AllVendor";
        vendors.push_back(vreg);

        std::vector<sdk::ProtocolRegistration> protocols;
        sdk::ProtocolRegistration preg;
        preg.name = "AllProtocol";
        preg.protocolType = mbootcore::discovery::ProtocolType::Sahara;
        protocols.push_back(preg);

        auto result = validator.validateAll(manifest, vendors, protocols,
            {}, {}, {}, {}, {}, {});
        REQUIRE(result.valid);
    }

    SECTION("testInvalidVendorName") {
        sdk::PluginValidator validator;
        sdk::VendorRegistration reg;
        reg.name = "";

        auto result = validator.validateVendor(reg);
        REQUIRE(!result.valid);
    }

    SECTION("testEmptyManifestInvalid") {
        sdk::PluginManifest manifest;
        manifest.setPluginName("");
        manifest.setPluginVersion("");

        REQUIRE(!manifest.isValid());

        auto errors = manifest.validate();
        REQUIRE(errors.size() >= 1);
    }
}
