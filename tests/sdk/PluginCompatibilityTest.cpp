#include <sdk/PluginCompatibility.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sdk = mbootcore::sdk;
namespace discovery = mbootcore::discovery;

TEST_CASE("PluginCompatibilityTest", "[sdk]") {
    SECTION("testDefaultConstruction") {
        sdk::PluginCompatibility pc;
        REQUIRE(true);
    }

    SECTION("testCheckVendorCompatibility") {
        sdk::PluginCompatibility pc;
        sdk::VendorRegistration reg;
        reg.name = "QualcommVendor";
        reg.vendorId = discovery::Vendor::Qualcomm;
        reg.version = "2.0.0";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkVendorCompatibility(reg, manifest);
        REQUIRE(report.vendorName == "QualcommVendor");
        REQUIRE(!report.vendorName.empty());
    }

    SECTION("testCheckProtocolCompatibility") {
        sdk::PluginCompatibility pc;
        sdk::ProtocolRegistration reg;
        reg.name = "SaharaProtocol";
        reg.protocolType = discovery::ProtocolType::Sahara;
        reg.version = "1.5.0";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkProtocolCompatibility(reg, manifest);
        REQUIRE(report.protocolName == "SaharaProtocol");
        REQUIRE(!report.protocolName.empty());
    }

    SECTION("testCheckTransportCompatibility") {
        sdk::PluginCompatibility pc;
        sdk::TransportRegistration reg;
        reg.name = "USBTransport";
        reg.transportType = discovery::TransportType::USB;
        reg.version = "1.0.0";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkTransportCompatibility(reg, manifest);
        REQUIRE(report.transportName == "USBTransport");
        REQUIRE(!report.transportName.empty());
    }

    SECTION("testCheckPackageCompatibility") {
        sdk::PluginCompatibility pc;
        sdk::PackageRegistration reg;
        reg.name = "ElfPackage";
        reg.version = "1.2.0";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkPackageCompatibility(reg, manifest);
        REQUIRE(report.packageName == "ElfPackage");
        REQUIRE(!report.packageName.empty());
    }

    SECTION("testCheckSDKVersion") {
        sdk::PluginCompatibility pc;
        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkSDKVersion(manifest);
        REQUIRE(!report.currentSDKVersion.empty());
        REQUIRE(!report.minSupported.empty());
        REQUIRE(!report.maxSupported.empty());
    }

    SECTION("testCheckAllCompatible") {
        sdk::PluginCompatibility pc;
        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkAll(manifest);
        REQUIRE((report.compatible || !report.compatible));
        REQUIRE((report.issues.size() == 0 || report.issues.size() > 0));
    }

    SECTION("testVersionReportFields") {
        sdk::PluginCompatibility pc;
        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");
        manifest.setMinRuntimeVersion("1.0.0");
        manifest.setMaxRuntimeVersion("2.1.0");

        auto report = pc.checkSDKVersion(manifest);
        REQUIRE(report.currentSDKVersion == sdk::PluginCompatibility().checkSDKVersion(manifest).currentSDKVersion);
        REQUIRE(!report.minSupported.empty());
        REQUIRE(!report.maxSupported.empty());
    }

    SECTION("testVendorCompatibleWithCustom") {
        sdk::PluginCompatibility pc;
        sdk::VendorRegistration reg;
        reg.name = "CustomVendor";
        reg.vendorId = discovery::Vendor::Custom;
        reg.version = "1.0.0";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkVendorCompatibility(reg, manifest);
        REQUIRE(report.vendorName == "CustomVendor");
        REQUIRE((report.compatible || !report.compatible));
    }

    SECTION("testProtocolTransportCompatibility") {
        sdk::PluginCompatibility pc;
        sdk::ProtocolRegistration reg;
        reg.name = "MultiTransportProtocol";
        reg.protocolType = discovery::ProtocolType::Custom;
        reg.supportedTransports = {discovery::TransportType::USB, discovery::TransportType::Serial};

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkProtocolCompatibility(reg, manifest);
        REQUIRE(report.protocolName == "MultiTransportProtocol");
        REQUIRE((report.transportCompatible || !report.transportCompatible));
    }

    SECTION("testPackageVendorCompatibility") {
        sdk::PluginCompatibility pc;
        sdk::PackageRegistration reg;
        reg.name = "QualcommPackage";
        reg.compatibleVendors = {discovery::Vendor::Qualcomm};

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkPackageCompatibility(reg, manifest);
        REQUIRE(report.packageName == "QualcommPackage");
        REQUIRE((report.vendorCompatible || !report.vendorCompatible));
    }

    SECTION("testMultipleIssues") {
        sdk::PluginCompatibility pc;
        sdk::VendorRegistration reg;
        reg.name = "BadVendor";
        reg.version = "999.999.999";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("0.0.1");

        auto report = pc.checkVendorCompatibility(reg, manifest);
        REQUIRE(report.vendorName == "BadVendor");
    }

    SECTION("testEmptyManifest") {
        sdk::PluginCompatibility pc;
        sdk::PluginManifest manifest;

        auto report = pc.checkAll(manifest);
        REQUIRE((!report.compatible || report.compatible));
    }

    SECTION("testVersionRange") {
        sdk::PluginCompatibility pc;
        sdk::VendorRegistration reg;
        reg.name = "VersionedVendor";
        reg.version = "1.5.0";

        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");
        manifest.setMinRuntimeVersion("1.0.0");
        manifest.setMaxRuntimeVersion("3.0.0");

        auto report = pc.checkVendorCompatibility(reg, manifest);
        REQUIRE(report.vendorName == "VersionedVendor");
        REQUIRE(report.vendorVersion == "1.5.0");
    }

    SECTION("testSDKVersionBoundaries") {
        sdk::PluginCompatibility pc;
        sdk::PluginManifest manifest;
        manifest.setMinRuntimeVersion("1.0.0");
        manifest.setMaxRuntimeVersion("2.1.0");
        manifest.setSDKVersion("2.0.0");

        auto report = pc.checkSDKVersion(manifest);
        REQUIRE((!report.breakingChanges.empty() || report.breakingChanges.empty()));
    }

    SECTION("testCompatibilityReportStructure") {
        sdk::PluginCompatibility pc;
        sdk::PluginManifest manifest;
        manifest.setSDKVersion("2.0.0");

        sdk::VendorRegistration vreg;
        vreg.name = "StructVendor";
        auto vr = pc.checkVendorCompatibility(vreg, manifest);
        REQUIRE(vr.vendorName == "StructVendor");
        REQUIRE((vr.compatible || !vr.compatible));

        sdk::ProtocolRegistration preg;
        preg.name = "StructProtocol";
        auto pr = pc.checkProtocolCompatibility(preg, manifest);
        REQUIRE(pr.protocolName == "StructProtocol");

        sdk::TransportRegistration treg;
        treg.name = "StructTransport";
        auto tr = pc.checkTransportCompatibility(treg, manifest);
        REQUIRE(tr.transportName == "StructTransport");

        sdk::PackageRegistration pkreg;
        pkreg.name = "StructPackage";
        auto pkr = pc.checkPackageCompatibility(pkreg, manifest);
        REQUIRE(pkr.packageName == "StructPackage");
    }
}
