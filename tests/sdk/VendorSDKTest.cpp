#include <sdk/VendorSDK.hpp>
#include <sdk/VendorSDKFactory.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <mbootcore/domain/Error.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;
namespace discovery = mbootcore::discovery;

TEST_CASE("VendorSDKTest", "[sdk]") {
    SECTION("testDefaultConstruction") {
        sdk::VendorSDK sdk;
        REQUIRE(!sdk.isFinalized());
        REQUIRE(sdk.vendors().size() == 0u);
        REQUIRE(sdk.protocols().size() == 0u);
        REQUIRE(sdk.transports().size() == 0u);
        REQUIRE(sdk.workflows().size() == 0u);
        REQUIRE(sdk.jobs().size() == 0u);
        REQUIRE(sdk.packages().size() == 0u);
        REQUIRE(sdk.discoveries().size() == 0u);
        REQUIRE(sdk.capabilities().size() == 0u);
    }

    SECTION("testRegisterSingleVendor") {
        sdk::VendorSDK sdk;
        sdk::VendorRegistration reg;
        reg.name = "TestVendor";
        reg.displayName = "Test Vendor Inc.";
        reg.version = "1.0.0";
        reg.vendorId = discovery::Vendor::Custom;
        sdk.registerVendor(reg);
        REQUIRE(sdk.vendors().size() == 1u);
        REQUIRE(sdk.vendors()[0].name == "TestVendor");
    }

    SECTION("testRegisterSingleProtocol") {
        sdk::VendorSDK sdk;
        sdk::ProtocolRegistration reg;
        reg.name = "TestProtocol";
        reg.displayName = "Test Protocol";
        reg.version = "1.0.0";
        reg.protocolType = discovery::ProtocolType::Custom;
        sdk.registerProtocol(reg);
        REQUIRE(sdk.protocols().size() == 1u);
        REQUIRE(sdk.protocols()[0].name == "TestProtocol");
    }

    SECTION("testRegisterSingleTransport") {
        sdk::VendorSDK sdk;
        sdk::TransportRegistration reg;
        reg.name = "TestTransport";
        reg.displayName = "Test Transport";
        reg.transportType = discovery::TransportType::Virtual;
        sdk.registerTransport(reg);
        REQUIRE(sdk.transports().size() == 1u);
        REQUIRE(sdk.transports()[0].name == "TestTransport");
    }

    SECTION("testRegisterSingleWorkflow") {
        sdk::VendorSDK sdk;
        sdk::WorkflowRegistration reg;
        reg.name = "TestWorkflow";
        reg.displayName = "Test Workflow";
        reg.version = "1.0.0";
        sdk.registerWorkflow(reg);
        REQUIRE(sdk.workflows().size() == 1u);
        REQUIRE(sdk.workflows()[0].name == "TestWorkflow");
    }

    SECTION("testRegisterSingleJob") {
        sdk::VendorSDK sdk;
        sdk::JobRegistration reg;
        reg.name = "TestJob";
        reg.displayName = "Test Job";
        reg.jobType = "flash";
        sdk.registerJob(reg);
        REQUIRE(sdk.jobs().size() == 1u);
        REQUIRE(sdk.jobs()[0].name == "TestJob");
    }

    SECTION("testRegisterSinglePackage") {
        sdk::VendorSDK sdk;
        sdk::PackageRegistration reg;
        reg.name = "TestPackage";
        reg.displayName = "Test Package Format";
        sdk.registerPackage(reg);
        REQUIRE(sdk.packages().size() == 1u);
        REQUIRE(sdk.packages()[0].name == "TestPackage");
    }

    SECTION("testRegisterSingleDiscovery") {
        sdk::VendorSDK sdk;
        sdk::DiscoveryRegistration reg;
        reg.name = "TestDiscovery";
        reg.displayName = "Test Discovery Module";
        sdk.registerDiscovery(reg);
        REQUIRE(sdk.discoveries().size() == 1u);
        REQUIRE(sdk.discoveries()[0].name == "TestDiscovery");
    }

    SECTION("testRegisterSingleCapability") {
        sdk::VendorSDK sdk;
        sdk::CapabilityRegistration reg;
        reg.name = "TestCapability";
        reg.displayName = "Test Capability";
        reg.capability = mbootcore::plugin::PluginCapability::Protocol;
        sdk.registerCapability(reg);
        REQUIRE(sdk.capabilities().size() == 1u);
        REQUIRE(sdk.capabilities()[0].name == "TestCapability");
    }

    SECTION("testRegisterMultipleVendors") {
        sdk::VendorSDK sdk;
        std::vector<sdk::VendorRegistration> regs;
        for (int i = 0; i < 5; ++i) {
            sdk::VendorRegistration reg;
            reg.name = "Vendor" + std::to_string(i);
            regs.push_back(reg);
        }
        sdk.registerVendors(regs);
        REQUIRE(sdk.vendors().size() == 5u);
    }

    SECTION("testFinalizeEmpty") {
        sdk::VendorSDK sdk;
        auto report = sdk.finalize();
        REQUIRE(sdk.isFinalized());
        REQUIRE(report.valid);
    }

    SECTION("testFinalizeWithRegistrations") {
        sdk::VendorSDK sdk;
        sdk::VendorRegistration vreg;
        vreg.name = "MyVendor";
        vreg.version = "1.0.0";
        sdk.registerVendor(vreg);

        sdk::ProtocolRegistration preg;
        preg.name = "MyProtocol";
        preg.protocolType = discovery::ProtocolType::Custom;
        sdk.registerProtocol(preg);

        auto report = sdk.finalize();
        REQUIRE(sdk.isFinalized());
        REQUIRE(report.registeredVendors.size() == 1u);
        REQUIRE(report.registeredProtocols.size() == 1u);
        REQUIRE(report.valid);
    }

    SECTION("testDuplicateVendorRegistration") {
        sdk::VendorSDK sdk;
        sdk::VendorRegistration reg;
        reg.name = "DupVendor";
        sdk.registerVendor(reg);
        sdk.registerVendor(reg);
        auto report = sdk.finalize();
        REQUIRE(!report.valid);
    }

    SECTION("testFactoryDefault") {
        auto sdk = sdk::VendorSDKFactory::createDefault();
        REQUIRE(sdk != nullptr);
        REQUIRE(!sdk->isFinalized());
    }

    SECTION("testFactoryMinimal") {
        auto sdk = sdk::VendorSDKFactory::createMinimal();
        REQUIRE(sdk != nullptr);
        REQUIRE(sdk->vendors().size() == 0u);
    }

    SECTION("testClearAfterRegistration") {
        sdk::VendorSDK sdk;
        sdk::VendorRegistration reg;
        reg.name = "ClearVendor";
        sdk.registerVendor(reg);
        REQUIRE(sdk.vendors().size() == 1u);
        sdk.clear();
        REQUIRE(sdk.vendors().size() == 0u);
        REQUIRE(!sdk.isFinalized());
    }
}
