/// Qualcomm Vendor Plugin Example
/// Demonstrates how a third-party developer creates a Qualcomm vendor plugin
/// using the VendorSDK.

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

    sdk::VendorRegistration qualcomm;
    qualcomm.name = "Qualcomm";
    qualcomm.displayName = "Qualcomm Technologies Inc.";
    qualcomm.description = "Qualcomm Snapdragon boot ROM protocol support";
    qualcomm.version = "2.1.0";
    qualcomm.vendorId = discovery::Vendor::Qualcomm;
    qualcomm.author = "MBootCore Team";
    qualcomm.license = "MIT";
    qualcomm.usbVids.push_back(0x05C6);
    qualcomm.usbVids.push_back(0x05C7);
    qualcomm.supportedProtocols.push_back(discovery::ProtocolType::Sahara);
    qualcomm.supportedProtocols.push_back(discovery::ProtocolType::Firehose);
    qualcomm.requiresSignedFirmware = true;
    vendorSdk->registerVendor(qualcomm);

    sdk::ProtocolRegistration sahara;
    sahara.name = "Sahara";
    sahara.displayName = "Sahara Protocol";
    sahara.description = "Qualcomm Sahara boot ROM streaming protocol";
    sahara.version = "2.0.0";
    sahara.protocolType = discovery::ProtocolType::Sahara;
    sahara.vendor = discovery::Vendor::Qualcomm;
    sahara.supportedBootModes.push_back(discovery::BootMode::EDL);
    sahara.supportedBootModes.push_back(discovery::BootMode::BootROM);
    sahara.supportedTransports.push_back(discovery::TransportType::USB);
    sahara.maxPacketSize = 16384;
    sahara.defaultTimeoutMs = 10000;
    sahara.supportsChecksum = true;
    vendorSdk->registerProtocol(sahara);

    sdk::ProtocolRegistration firehose;
    firehose.name = "Firehose";
    firehose.displayName = "Firehose Protocol";
    firehose.description = "Qualcomm Firehose XML-based flashing protocol";
    firehose.version = "1.5.0";
    firehose.protocolType = discovery::ProtocolType::Firehose;
    firehose.vendor = discovery::Vendor::Qualcomm;
    firehose.supportedBootModes.push_back(discovery::BootMode::Firehose);
    firehose.supportedTransports.push_back(discovery::TransportType::USB);
    firehose.supportsMultiPacket = true;
    vendorSdk->registerProtocol(firehose);

    sdk::TransportRegistration usbTransport;
    usbTransport.name = "QualcommUSB";
    usbTransport.displayName = "Qualcomm USB Transport";
    usbTransport.description = "USB transport for Qualcomm devices";
    usbTransport.version = "1.0.0";
    usbTransport.transportType = discovery::TransportType::USB;
    usbTransport.compatibleProtocols.push_back(discovery::ProtocolType::Sahara);
    usbTransport.compatibleProtocols.push_back(discovery::ProtocolType::Firehose);
    usbTransport.supportsHotplug = true;
    usbTransport.supportsReconnect = true;
    vendorSdk->registerTransport(usbTransport);

    sdk::WorkflowRegistration edlWorkflow;
    edlWorkflow.name = "EDLFlash";
    edlWorkflow.displayName = "EDL Flashing Workflow";
    edlWorkflow.description = "Complete EDL mode device flashing workflow";
    edlWorkflow.version = "1.0.0";
    edlWorkflow.compatibleVendors.push_back("Qualcomm");
    edlWorkflow.compatibleProtocols.push_back("Sahara");
    edlWorkflow.compatibleProtocols.push_back("Firehose");
    edlWorkflow.isDefault = true;
    edlWorkflow.priority = 90;
    vendorSdk->registerWorkflow(edlWorkflow);

    sdk::JobRegistration flashJob;
    flashJob.name = "QualcommFlash";
    flashJob.displayName = "Qualcomm Flash Job";
    flashJob.description = "Flash firmware to Qualcomm device";
    flashJob.version = "1.0.0";
    flashJob.jobType = "flash";
    flashJob.compatibleVendors.push_back("Qualcomm");
    flashJob.compatibleProtocols.push_back("Firehose");
    flashJob.supportsCancellation = true;
    flashJob.supportsProgress = true;
    vendorSdk->registerJob(flashJob);

    sdk::DiscoveryRegistration qcDiscovery;
    qcDiscovery.name = "QualcommDiscovery";
    qcDiscovery.displayName = "Qualcomm Device Discovery";
    qcDiscovery.description = "USB-based device discovery for Qualcomm devices";
    qcDiscovery.version = "1.0.0";
    qcDiscovery.vendor = discovery::Vendor::Qualcomm;
    qcDiscovery.usbVids.push_back(0x05C6);
    qcDiscovery.usbVids.push_back(0x05C7);
    qcDiscovery.discoveryMethods.push_back("USB");
    qcDiscovery.discoveryMethods.push_back("VID_PID_MATCH");
    vendorSdk->registerDiscovery(qcDiscovery);

    sdk::CapabilityRegistration saharaCap;
    saharaCap.name = "SaharaProtocol";
    saharaCap.displayName = "Sahara Protocol Support";
    saharaCap.description = "Support for Qualcomm Sahara protocol";
    saharaCap.version = "1.0.0";
    saharaCap.capability = plugin::PluginCapability::Protocol;
    saharaCap.isRequired = true;
    vendorSdk->registerCapability(saharaCap);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Qualcomm vendor SDK registration successful!" << std::endl;
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
    manifest.setPluginName("QualcommVendorPlugin");
    manifest.setPluginVersion("2.1.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("Qualcomm");
    manifest.setAuthor("MBootCore Team");
    manifest.setLicense("MIT");
    manifest.setDescription("Qualcomm Snapdragon boot ROM protocol support");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
