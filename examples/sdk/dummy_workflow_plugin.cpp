/// Dummy Workflow Plugin Example
/// Demonstrates registering a custom workflow with the VendorSDK.

#include <sdk/VendorSDK.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
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
    vendor.name = "WorkflowDemo";
    vendor.displayName = "Workflow Demo Vendor";
    vendor.description = "Vendor demonstrating workflow registration";
    vendor.version = "1.0.0";
    vendor.vendorId = discovery::Vendor::Custom;
    vendor.author = "Demo Author";
    vendor.license = "BSD-2";
    vendor.usbVids.push_back(0xBEEF);
    vendor.requiresSignedFirmware = false;
    vendorSdk->registerVendor(vendor);

    sdk::ProtocolRegistration protocol;
    protocol.name = "DemoProtocol";
    protocol.displayName = "Demo Protocol";
    protocol.description = "A protocol for workflow demonstration";
    protocol.version = "1.0.0";
    protocol.protocolType = discovery::ProtocolType::Custom;
    protocol.vendor = discovery::Vendor::Custom;
    protocol.supportedBootModes.push_back(discovery::BootMode::Unknown);
    protocol.supportedTransports.push_back(discovery::TransportType::USB);
    protocol.maxPacketSize = 512;
    protocol.defaultTimeoutMs = 3000;
    vendorSdk->registerProtocol(protocol);

    sdk::WorkflowRegistration workflow;
    workflow.name = "DemoWorkflow";
    workflow.displayName = "Demo Flashing Workflow";
    workflow.description = "A demonstration workflow for custom devices";
    workflow.version = "1.0.0";
    workflow.compatibleVendors.push_back("WorkflowDemo");
    workflow.compatibleProtocols.push_back("DemoProtocol");
    workflow.isDefault = true;
    workflow.priority = 50;
    vendorSdk->registerWorkflow(workflow);

    sdk::DiscoveryRegistration discovery;
    discovery.name = "WorkflowDemoDiscovery";
    discovery.displayName = "Workflow Demo Discovery";
    discovery.description = "Discovery for workflow demo devices";
    discovery.version = "1.0.0";
    discovery.vendor = discovery::Vendor::Custom;
    discovery.usbVids.push_back(0xBEEF);
    discovery.discoveryMethods.push_back("USB");
    vendorSdk->registerDiscovery(discovery);

    sdk::CapabilityRegistration cap;
    cap.name = "WorkflowDemoCap";
    cap.displayName = "Workflow Demo Support";
    cap.description = "Support for workflow demo";
    cap.version = "1.0.0";
    cap.capability = plugin::PluginCapability::PipelineStage;
    cap.isRequired = true;
    vendorSdk->registerCapability(cap);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Dummy workflow SDK registration successful!" << std::endl;
        std::cout << "Registered " << report.registeredWorkflows.size() << " workflow(s)" << std::endl;
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
    manifest.setPluginName("DemoWorkflowPlugin");
    manifest.setPluginVersion("1.0.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("WorkflowDemo");
    manifest.setAuthor("Demo Author");
    manifest.setLicense("BSD-2");
    manifest.setDescription("Demonstration custom workflow plugin");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
