/// Dummy Package Plugin Example
/// Demonstrates registering a custom firmware package format with the VendorSDK.

#include <sdk/VendorSDK.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/VendorSDKFactory.hpp>

#include <iostream>

namespace sdk = mbootcore::sdk;
namespace discovery = mbootcore::discovery;
namespace plugin = mbootcore::plugin;

int main() {
    auto vendorSdk = sdk::VendorSDKFactory::createDefault();

    sdk::VendorRegistration vendor;
    vendor.name = "PackageDemo";
    vendor.displayName = "Package Demo Vendor";
    vendor.description = "Vendor demonstrating package format registration";
    vendor.version = "1.0.0";
    vendor.vendorId = discovery::Vendor::Custom;
    vendor.author = "Demo Author";
    vendor.license = "Apache-2.0";
    vendor.usbVids.push_back(0xCAFE);
    vendor.requiresSignedFirmware = false;
    vendorSdk->registerVendor(vendor);

    sdk::ProtocolRegistration protocol;
    protocol.name = "PackageDemoProtocol";
    protocol.displayName = "Package Demo Protocol";
    protocol.description = "Minimal protocol for package demo";
    protocol.version = "1.0.0";
    protocol.protocolType = discovery::ProtocolType::Custom;
    protocol.vendor = discovery::Vendor::Custom;
    protocol.supportedBootModes.push_back(discovery::BootMode::Unknown);
    protocol.supportedTransports.push_back(discovery::TransportType::USB);
    protocol.maxPacketSize = 256;
    protocol.defaultTimeoutMs = 5000;
    vendorSdk->registerProtocol(protocol);

    sdk::PackageRegistration package;
    package.name = "CustomPackageFormat";
    package.displayName = "Custom Package Format";
    package.description = "A custom firmware package format for demonstration";
    package.version = "1.0.0";
    package.compatibleVendors.push_back(discovery::Vendor::Custom);
    package.compatibleProtocols.push_back(discovery::ProtocolType::Custom);
    package.supportedFileExtensions.push_back(".cpkg");
    package.supportedFileExtensions.push_back(".custom");
    package.supportsCompression = true;
    package.requiresSignature = false;
    vendorSdk->registerPackage(package);

    sdk::DiscoveryRegistration discovery;
    discovery.name = "PackageDemoDiscovery";
    discovery.displayName = "Package Demo Discovery";
    discovery.description = "Discovery for package demo devices";
    discovery.version = "1.0.0";
    discovery.vendor = discovery::Vendor::Custom;
    discovery.usbVids.push_back(0xCAFE);
    discovery.discoveryMethods.push_back("USB");
    vendorSdk->registerDiscovery(discovery);

    sdk::CapabilityRegistration cap;
    cap.name = "PackageFormatCap";
    cap.displayName = "Custom Package Format Support";
    cap.description = "Support for the custom firmware package format";
    cap.version = "1.0.0";
    cap.capability = plugin::PluginCapability::None;
    cap.isRequired = true;
    vendorSdk->registerCapability(cap);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Dummy package SDK registration successful!" << std::endl;
        std::cout << "Registered " << report.registeredPackages.size() << " package format(s)" << std::endl;
        std::cout << "Registered " << report.registeredProtocols.size() << " protocol(s)" << std::endl;
        std::cout << "Registered " << report.registeredDiscoveries.size() << " discovery module(s)" << std::endl;
    } else {
        std::cerr << "SDK registration failed:" << std::endl;
        for (const auto& err : report.errors) {
            std::cerr << "  - " << err << std::endl;
        }
        return 1;
    }

    sdk::PluginManifest manifest;
    manifest.setPluginName("CustomPackagePlugin");
    manifest.setPluginVersion("1.0.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("PackageDemo");
    manifest.setAuthor("Demo Author");
    manifest.setLicense("Apache-2.0");
    manifest.setDescription("Demonstration custom firmware package format plugin");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
