/// Dummy Protocol Plugin Example
/// Demonstrates registering a custom protocol type with the VendorSDK.

#include <sdk/VendorSDK.hpp>
#include <sdk/ProtocolRegistration.hpp>
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
    vendor.name = "DummyProtocolVendor";
    vendor.displayName = "Dummy Protocol Vendor";
    vendor.description = "Vendor for custom protocol demonstration";
    vendor.version = "0.1.0";
    vendor.vendorId = discovery::Vendor::Custom;
    vendor.author = "Demo Author";
    vendor.license = "MIT";
    vendor.usbVids.push_back(0xDEAD);
    vendor.requiresSignedFirmware = false;
    vendorSdk->registerVendor(vendor);

    sdk::ProtocolRegistration protocol;
    protocol.name = "DummyProtocol";
    protocol.displayName = "Dummy Protocol";
    protocol.description = "A demonstration custom protocol";
    protocol.version = "0.1.0";
    protocol.protocolType = discovery::ProtocolType::Custom;
    protocol.vendor = discovery::Vendor::Custom;
    protocol.supportedBootModes.push_back(discovery::BootMode::Unknown);
    protocol.supportedTransports.push_back(discovery::TransportType::USB);
    protocol.maxPacketSize = 1024;
    protocol.defaultTimeoutMs = 5000;
    protocol.supportsChecksum = false;
    vendorSdk->registerProtocol(protocol);

    sdk::DiscoveryRegistration discovery;
    discovery.name = "DummyDiscovery";
    discovery.displayName = "Dummy Device Discovery";
    discovery.description = "Discovery for dummy protocol devices";
    discovery.version = "0.1.0";
    discovery.vendor = discovery::Vendor::Custom;
    discovery.usbVids.push_back(0xDEAD);
    discovery.discoveryMethods.push_back("USB");
    vendorSdk->registerDiscovery(discovery);

    sdk::CapabilityRegistration cap;
    cap.name = "DummyProtocolCap";
    cap.displayName = "Dummy Protocol Support";
    cap.description = "Support for the dummy custom protocol";
    cap.version = "0.1.0";
    cap.capability = plugin::PluginCapability::Protocol;
    cap.isRequired = true;
    vendorSdk->registerCapability(cap);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Dummy protocol SDK registration successful!" << std::endl;
        std::cout << "Registered " << report.registeredProtocols.size() << " protocol(s)" << std::endl;
        std::cout << "Registered " << report.registeredDiscoveries.size() << " discovery module(s)" << std::endl;
        std::cout << "Registered " << report.registeredCapabilities.size() << " capability(ies)" << std::endl;
    } else {
        std::cerr << "SDK registration failed:" << std::endl;
        for (const auto& err : report.errors) {
            std::cerr << "  - " << err << std::endl;
        }
        return 1;
    }

    sdk::PluginManifest manifest;
    manifest.setPluginName("DummyProtocolPlugin");
    manifest.setPluginVersion("0.1.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("DummyProtocolVendor");
    manifest.setAuthor("Demo Author");
    manifest.setLicense("MIT");
    manifest.setDescription("Demonstration custom protocol plugin");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
