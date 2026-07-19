/// Vendor SDK Registration Example
/// Demonstrates how a third-party developer registers a vendor plugin
/// using the VendorSDK. The MediaTek registration shown here is a
/// HYPOTHETICAL EXAMPLE — MediaTek is a reference scaffold, not a
/// production implementation.

#include <sdk/VendorSDK.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/VendorSDKFactory.hpp>

#include <iostream>
#include <cassert>

namespace sdk = mbootcore::sdk;
namespace discovery = mbootcore::discovery;
namespace plugin = mbootcore::plugin;

int main() {
    auto vendorSdk = sdk::VendorSDKFactory::createDefault();

    // NOTE: MediaTek is a reference scaffold, not a production implementation.
    // This registration demonstrates the SDK pattern only.
    sdk::VendorRegistration mediatek;
    mediatek.name = "MediaTek";
    mediatek.displayName = "MediaTek Inc.";
    mediatek.description = "MediaTek boot ROM protocol (scaffold — not production)";
    mediatek.version = "1.3.0";
    mediatek.vendorId = discovery::Vendor::MediaTek;
    mediatek.author = "MBootCore Team";
    mediatek.license = "MIT";
    mediatek.usbVids.push_back(0x0E8D);
    mediatek.supportedProtocols.push_back(discovery::ProtocolType::MediaTekBROM);
    mediatek.supportedProtocols.push_back(discovery::ProtocolType::MediaTekDA);
    mediatek.requiresSignedFirmware = false;
    vendorSdk->registerVendor(mediatek);

    sdk::ProtocolRegistration brom;
    brom.name = "MediaTekBROM";
    brom.displayName = "MediaTek BROM Protocol";
    brom.description = "MediaTek BootROM serial protocol for preloader mode";
    brom.version = "1.2.0";
    brom.protocolType = discovery::ProtocolType::MediaTekBROM;
    brom.vendor = discovery::Vendor::MediaTek;
    brom.supportedBootModes.push_back(discovery::BootMode::Preloader);
    brom.supportedBootModes.push_back(discovery::BootMode::BootROM);
    brom.supportedTransports.push_back(discovery::TransportType::USB);
    brom.supportedTransports.push_back(discovery::TransportType::Serial);
    brom.maxPacketSize = 4096;
    brom.defaultTimeoutMs = 15000;
    brom.supportsChecksum = true;
    vendorSdk->registerProtocol(brom);

    sdk::ProtocolRegistration da;
    da.name = "MediaTekDA";
    da.displayName = "MediaTek DA Protocol";
    da.description = "MediaTek Download Agent protocol for flashing";
    da.version = "1.1.1";
    da.protocolType = discovery::ProtocolType::MediaTekDA;
    da.vendor = discovery::Vendor::MediaTek;
    da.supportedBootModes.push_back(discovery::BootMode::DownloadMode);
    da.supportedTransports.push_back(discovery::TransportType::USB);
    da.supportsMultiPacket = true;
    da.supportsChecksum = true;
    vendorSdk->registerProtocol(da);

    sdk::TransportRegistration usbTransport;
    usbTransport.name = "MediaTekUSB";
    usbTransport.displayName = "MediaTek USB Transport";
    usbTransport.description = "USB transport for MediaTek devices";
    usbTransport.version = "1.0.0";
    usbTransport.transportType = discovery::TransportType::USB;
    usbTransport.compatibleProtocols.push_back(discovery::ProtocolType::MediaTekBROM);
    usbTransport.compatibleProtocols.push_back(discovery::ProtocolType::MediaTekDA);
    usbTransport.supportsHotplug = true;
    usbTransport.supportsReconnect = false;
    vendorSdk->registerTransport(usbTransport);

    sdk::WorkflowRegistration bromWorkflow;
    bromWorkflow.name = "BROMFlash";
    bromWorkflow.displayName = "BROM Flashing Workflow";
    bromWorkflow.description = "Preloader mode device flashing via BROM + DA";
    bromWorkflow.version = "1.0.0";
    bromWorkflow.compatibleVendors.push_back("MediaTek");
    bromWorkflow.compatibleProtocols.push_back("MediaTekBROM");
    bromWorkflow.compatibleProtocols.push_back("MediaTekDA");
    bromWorkflow.isDefault = true;
    bromWorkflow.priority = 85;
    vendorSdk->registerWorkflow(bromWorkflow);

    sdk::JobRegistration flashJob;
    flashJob.name = "MediaTekFlash";
    flashJob.displayName = "MediaTek Flash Job";
    flashJob.description = "Flash firmware to MediaTek device";
    flashJob.version = "1.0.0";
    flashJob.jobType = "flash";
    flashJob.compatibleVendors.push_back("MediaTek");
    flashJob.compatibleProtocols.push_back("MediaTekDA");
    flashJob.supportsCancellation = true;
    flashJob.supportsProgress = true;
    vendorSdk->registerJob(flashJob);

    sdk::DiscoveryRegistration mtkDiscovery;
    mtkDiscovery.name = "MediaTekDiscovery";
    mtkDiscovery.displayName = "MediaTek Device Discovery";
    mtkDiscovery.description = "USB-based device discovery for MediaTek devices";
    mtkDiscovery.version = "1.0.0";
    mtkDiscovery.vendor = discovery::Vendor::MediaTek;
    mtkDiscovery.usbVids.push_back(0x0E8D);
    mtkDiscovery.discoveryMethods.push_back("USB");
    mtkDiscovery.discoveryMethods.push_back("VID_PID_MATCH");
    vendorSdk->registerDiscovery(mtkDiscovery);

    sdk::CapabilityRegistration bromCap;
    bromCap.name = "BROMProtocol";
    bromCap.displayName = "BROM Protocol Support";
    bromCap.description = "Support for MediaTek BROM protocol";
    bromCap.version = "1.0.0";
    bromCap.capability = plugin::PluginCapability::Protocol;
    bromCap.isRequired = true;
    vendorSdk->registerCapability(bromCap);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Vendor SDK registration successful!" << std::endl;
        std::cout << "Registered " << report.registeredVendors.size() << " vendor(s)" << std::endl;
        std::cout << "Registered " << report.registeredProtocols.size() << " protocol(s)" << std::endl;
        std::cout << "Registered " << report.registeredTransports.size() << " transport(s)" << std::endl;
        std::cout << "Registered " << report.registeredWorkflows.size() << " workflow(s)" << std::endl;
        std::cout << "Registered " << report.registeredJobs.size() << " job(s)" << std::endl;
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
    manifest.setPluginName("MediaTekVendorPlugin");
    manifest.setPluginVersion("1.3.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("MediaTek");
    manifest.setAuthor("MBootCore Team");
    manifest.setLicense("MIT");
    manifest.setDescription("MediaTek boot ROM protocol support (BROM + DA)");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
