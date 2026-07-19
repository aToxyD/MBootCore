/// Dummy Discovery Plugin Example
/// Demonstrates registering a custom discovery module with the VendorSDK.

#include <sdk/VendorSDK.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
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
    vendor.name = "DiscoveryDemo";
    vendor.displayName = "Discovery Demo Vendor";
    vendor.description = "Vendor demonstrating custom discovery registration";
    vendor.version = "1.0.0";
    vendor.vendorId = discovery::Vendor::Custom;
    vendor.author = "Demo Author";
    vendor.license = "MIT";
    vendor.usbVids.push_back(0xABBA);
    vendor.usbVids.push_back(0xABBB);
    vendor.supportedProtocols.push_back(discovery::ProtocolType::Custom);
    vendor.requiresSignedFirmware = false;
    vendorSdk->registerVendor(vendor);

    sdk::ProtocolRegistration protocol;
    protocol.name = "DiscoveryTargetProtocol";
    protocol.displayName = "Discovery Target Protocol";
    protocol.description = "Protocol targeted by custom discovery";
    protocol.version = "1.0.0";
    protocol.protocolType = discovery::ProtocolType::Custom;
    protocol.vendor = discovery::Vendor::Custom;
    protocol.supportedBootModes.push_back(discovery::BootMode::Unknown);
    protocol.supportedTransports.push_back(discovery::TransportType::USB);
    protocol.supportedTransports.push_back(discovery::TransportType::TCP);
    protocol.maxPacketSize = 1024;
    protocol.defaultTimeoutMs = 2000;
    vendorSdk->registerProtocol(protocol);

    sdk::TransportRegistration transport;
    transport.name = "DiscoveryDemoTransport";
    transport.displayName = "Discovery Demo Transport";
    transport.description = "Transport for discovery demo devices";
    transport.version = "1.0.0";
    transport.transportType = discovery::TransportType::USB;
    transport.compatibleProtocols.push_back(discovery::ProtocolType::Custom);
    transport.supportsHotplug = true;
    transport.supportsReconnect = true;
    vendorSdk->registerTransport(transport);

    sdk::DiscoveryRegistration discoveryReg;
    discoveryReg.name = "CustomDiscovery";
    discoveryReg.displayName = "Custom Device Discovery";
    discoveryReg.description = "Multi-method discovery for custom devices";
    discoveryReg.version = "1.0.0";
    discoveryReg.vendor = discovery::Vendor::Custom;
    discoveryReg.usbVids.push_back(0xABBA);
    discoveryReg.usbVids.push_back(0xABBB);
    discoveryReg.usbPids.push_back(0x0001);
    discoveryReg.usbPids.push_back(0x0002);
    discoveryReg.usbPids.push_back(0x0003);
    discoveryReg.discoveryMethods.push_back("USB");
    discoveryReg.discoveryMethods.push_back("VID_PID_MATCH");
    discoveryReg.discoveryMethods.push_back("NETWORK_SCAN");
    discoveryReg.discoveryMethods.push_back("SERIAL_SCAN");
    vendorSdk->registerDiscovery(discoveryReg);

    sdk::CapabilityRegistration discoveryCap;
    discoveryCap.name = "CustomDiscoveryCap";
    discoveryCap.displayName = "Custom Discovery Support";
    discoveryCap.description = "Support for custom device discovery methods";
    discoveryCap.version = "1.0.0";
    discoveryCap.capability = plugin::PluginCapability::Discovery;
    discoveryCap.isRequired = true;
    vendorSdk->registerCapability(discoveryCap);

    sdk::CapabilityRegistration protocolCap;
    protocolCap.name = "DiscoveryTargetProtocolCap";
    protocolCap.displayName = "Discovery Target Protocol Support";
    protocolCap.description = "Support for the discovery target protocol";
    protocolCap.version = "1.0.0";
    protocolCap.capability = plugin::PluginCapability::Protocol;
    protocolCap.isRequired = true;
    vendorSdk->registerCapability(protocolCap);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Dummy discovery SDK registration successful!" << std::endl;
        std::cout << "Registered " << report.registeredDiscoveries.size() << " discovery module(s)" << std::endl;
        std::cout << "Registered " << report.registeredVendors.size() << " vendor(s)" << std::endl;
        std::cout << "Registered " << report.registeredProtocols.size() << " protocol(s)" << std::endl;
        std::cout << "Registered " << report.registeredTransports.size() << " transport(s)" << std::endl;
        std::cout << "Registered " << report.registeredCapabilities.size() << " capability(ies)" << std::endl;
    } else {
        std::cerr << "SDK registration failed:" << std::endl;
        for (const auto& err : report.errors) {
            std::cerr << "  - " << err << std::endl;
        }
        return 1;
    }

    sdk::PluginManifest manifest;
    manifest.setPluginName("CustomDiscoveryPlugin");
    manifest.setPluginVersion("1.0.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("DiscoveryDemo");
    manifest.setAuthor("Demo Author");
    manifest.setLicense("MIT");
    manifest.setDescription("Demonstration custom discovery module plugin");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
