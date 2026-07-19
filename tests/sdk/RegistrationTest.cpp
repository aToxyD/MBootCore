#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <sdk/VendorSDK.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;
namespace discovery = mbootcore::discovery;

TEST_CASE("RegistrationTest", "[sdk]") {
    SECTION("testVendorRegistrationDefaults") {
        sdk::VendorRegistration reg;
        REQUIRE(reg.vendorId == discovery::Vendor::Custom);
        REQUIRE(reg.name.empty());
        REQUIRE(reg.usbVids.empty());
        REQUIRE(reg.usbPids.empty());
    }

    SECTION("testProtocolRegistrationDefaults") {
        sdk::ProtocolRegistration reg;
        REQUIRE(reg.protocolType == discovery::ProtocolType::Custom);
        REQUIRE(reg.version.empty());
        REQUIRE(reg.supportedBootModes.empty());
    }

    SECTION("testTransportRegistrationDefaults") {
        sdk::TransportRegistration reg;
        REQUIRE(reg.transportType == discovery::TransportType::Virtual);
        REQUIRE(reg.description.empty());
    }

    SECTION("testWorkflowRegistrationDefaults") {
        sdk::WorkflowRegistration reg;
        REQUIRE(reg.name.empty());
        REQUIRE(reg.version.empty());
    }

    SECTION("testJobRegistrationDefaults") {
        sdk::JobRegistration reg;
        REQUIRE(reg.name.empty());
        REQUIRE(reg.jobType.empty());
    }

    SECTION("testPackageRegistrationDefaults") {
        sdk::PackageRegistration reg;
        REQUIRE(reg.name.empty());
        REQUIRE(reg.version.empty());
    }

    SECTION("testDiscoveryRegistrationDefaults") {
        sdk::DiscoveryRegistration reg;
        REQUIRE(reg.name.empty());
        REQUIRE(reg.scanTimeoutMs == uint32_t(5000));
    }

    SECTION("testCapabilityRegistrationDefaults") {
        sdk::CapabilityRegistration reg;
        REQUIRE(reg.requiredCapabilities.empty());
        REQUIRE(reg.name.empty());
    }

    SECTION("testVendorRegistrationCustomValues") {
        sdk::VendorRegistration reg;
        reg.vendorId = discovery::Vendor::Qualcomm;
        reg.name = "Qualcomm";
        reg.usbVids.push_back(0x05C6);
        reg.usbPids.push_back(0x9008);

        REQUIRE(reg.vendorId == discovery::Vendor::Qualcomm);
        REQUIRE(reg.name == std::string("Qualcomm"));
        REQUIRE(reg.usbVids.size() == size_t(1));
        REQUIRE(reg.usbVids[0] == 0x05C6);
        REQUIRE(reg.usbPids.size() == size_t(1));
        REQUIRE(reg.usbPids[0] == 0x9008);
    }

    SECTION("testProtocolRegistrationCustomValues") {
        sdk::ProtocolRegistration reg;
        reg.protocolType = discovery::ProtocolType::Sahara;
        reg.version = "2.1";
        reg.supportedBootModes.push_back(discovery::BootMode::EDL);

        REQUIRE(reg.protocolType == discovery::ProtocolType::Sahara);
        REQUIRE(reg.version == std::string("2.1"));
        REQUIRE(reg.supportedBootModes.size() == size_t(1));
        REQUIRE(reg.supportedBootModes[0] == discovery::BootMode::EDL);
    }

    SECTION("testTransportRegistrationCustomValues") {
        sdk::TransportRegistration reg;
        reg.transportType = discovery::TransportType::USB;
        reg.description = "USB Bulk Transport";

        REQUIRE(reg.transportType == discovery::TransportType::USB);
        REQUIRE(reg.description == std::string("USB Bulk Transport"));
    }

    SECTION("testChainedRegistration") {
        auto sdk = std::make_unique<sdk::VendorSDK>();

        sdk::VendorRegistration vreg;
        vreg.vendorId = discovery::Vendor::Qualcomm;
        vreg.name = "Qualcomm";
        sdk->registerVendor(vreg);

        sdk::ProtocolRegistration preg;
        preg.name = "SaharaProtocol";
        preg.protocolType = discovery::ProtocolType::Sahara;
        preg.version = "1.0";
        sdk->registerProtocol(preg);

        sdk::TransportRegistration treg;
        treg.name = "UsbTransport";
        treg.transportType = discovery::TransportType::USB;
        treg.description = "USB Transport";
        sdk->registerTransport(treg);

        auto report = sdk->finalize();
        REQUIRE(report.valid);
    }

    SECTION("testMixedRegistrationOrder") {
        auto sdk = std::make_unique<sdk::VendorSDK>();

        sdk::ProtocolRegistration preg1;
        preg1.name = "FirehoseProtocol";
        preg1.protocolType = discovery::ProtocolType::Firehose;
        preg1.version = "1.0";
        sdk->registerProtocol(preg1);

        sdk::VendorRegistration vreg;
        vreg.vendorId = discovery::Vendor::MediaTek;
        vreg.name = "MediaTek";
        sdk->registerVendor(vreg);

        sdk::TransportRegistration treg;
        treg.name = "SerialTransport";
        treg.transportType = discovery::TransportType::Serial;
        treg.description = "Serial Transport";
        sdk->registerTransport(treg);

        sdk::ProtocolRegistration preg2;
        preg2.name = "SaharaProtocol";
        preg2.protocolType = discovery::ProtocolType::Sahara;
        preg2.version = "2.0";
        sdk->registerProtocol(preg2);

        auto report = sdk->finalize();
        REQUIRE(report.valid);
    }

    SECTION("testLargeBatchRegistration") {
        auto sdk = std::make_unique<sdk::VendorSDK>();

        for (int i = 0; i < 100; ++i) {
            sdk::JobRegistration jreg;
            jreg.name = "Job" + std::to_string(i);
            jreg.jobType = "Batch";
            sdk->registerJob(jreg);
        }

        auto report = sdk->finalize();
        REQUIRE(report.valid);
    }

    SECTION("testRegistrationIdempotency") {
        auto sdk = std::make_unique<sdk::VendorSDK>();

        sdk::VendorRegistration reg1;
        reg1.vendorId = discovery::Vendor::Qualcomm;
        reg1.name = "Qualcomm";
        sdk->registerVendor(reg1);

        sdk::VendorRegistration reg2;
        reg2.vendorId = discovery::Vendor::Qualcomm;
        reg2.name = "Qualcomm";
        sdk->registerVendor(reg2);

        auto report = sdk->finalize();
        REQUIRE(!report.valid);
    }

    SECTION("testVendorUSBIds") {
        sdk::VendorRegistration reg;
        reg.vendorId = discovery::Vendor::Qualcomm;
        reg.name = "Qualcomm";
        reg.usbVids = {0x05C6, 0x0489};
        reg.usbPids = {0x9008, 0x900E};

        REQUIRE(reg.usbVids.size() == size_t(2));
        REQUIRE(reg.usbPids.size() == size_t(2));
        REQUIRE(reg.usbVids[0] == 0x05C6);
        REQUIRE(reg.usbVids[1] == 0x0489);
        REQUIRE(reg.usbPids[0] == 0x9008);
        REQUIRE(reg.usbPids[1] == 0x900E);
    }

    SECTION("testProtocolBootModes") {
        sdk::ProtocolRegistration reg;
        reg.protocolType = discovery::ProtocolType::Sahara;
        reg.supportedBootModes = {discovery::BootMode::EDL,
                                  discovery::BootMode::Fastboot,
                                  discovery::BootMode::DownloadMode};

        REQUIRE(reg.supportedBootModes.size() == size_t(3));
        REQUIRE(reg.supportedBootModes[0] == discovery::BootMode::EDL);
        REQUIRE(reg.supportedBootModes[1] == discovery::BootMode::Fastboot);
        REQUIRE(reg.supportedBootModes[2] == discovery::BootMode::DownloadMode);
    }
}
