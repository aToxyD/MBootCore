#include <mbootcore/MBootCore.hpp>
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
#include <sdk/Version.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <iostream>
#include <cstdlib>
#include <memory>

using namespace mbootcore;

int main() {
    auto logger = std::make_shared<ConsoleLogger>();
    logger->info("example", "MBootCore Vendor SDK Example");
    logger->info("example", "SDK Version: " + sdk::getSDKVersion().toString());

    // Create VendorSDK via factory
    auto sdk = sdk::VendorSDKFactory::createDefault();
    logger->info("example", "Default VendorSDK created");

    // Register a custom vendor
    sdk::VendorRegistration vendorReg;
    vendorReg.name = "acme-corp";
    vendorReg.displayName = "ACME Corp";
    vendorReg.description = "ACME Corp BootROM devices";
    vendorReg.version = "1.0.0";
    vendorReg.vendorId = discovery::Vendor::Custom;
    vendorReg.sdkVersion = "2.0.0";
    vendorReg.author = "ACME Engineering";
    vendorReg.supportedProtocols = {discovery::ProtocolType::Custom};
    vendorReg.usbVids = {0x1234};
    vendorReg.usbPids = {0x5678};
    vendorReg.supportsMultipleFlash = true;

    sdk->registerVendor(vendorReg);
    logger->info("example", "Registered vendor: " + vendorReg.displayName);

    // Register a custom protocol
    sdk::ProtocolRegistration protoReg;
    protoReg.name = "acme-protocol";
    protoReg.displayName = "ACME Flash Protocol";
    protoReg.description = "ACME Corp custom flash protocol";
    protoReg.version = "1.0.0";
    protoReg.protocolType = discovery::ProtocolType::Custom;
    protoReg.vendor = discovery::Vendor::Custom;
    protoReg.supportedBootModes = {discovery::BootMode::DownloadMode};
    protoReg.supportedTransports = {discovery::TransportType::USB};
    protoReg.maxPacketSize = 4096;
    protoReg.defaultTimeoutMs = 10000;

    sdk->registerProtocol(protoReg);
    logger->info("example", "Registered protocol: " + protoReg.displayName);

    // Register a custom transport
    sdk::TransportRegistration transportReg;
    transportReg.name = "acme-usb";
    transportReg.displayName = "ACME USB Transport";
    transportReg.description = "ACME Corp USB HID transport";
    transportReg.version = "1.0.0";
    transportReg.transportType = discovery::TransportType::USB;
    transportReg.compatibleProtocols = {discovery::ProtocolType::Custom};
    transportReg.maxTransferSize = 65536;
    transportReg.supportsHotplug = true;
    transportReg.supportsReconnect = true;

    sdk->registerTransport(transportReg);
    logger->info("example", "Registered transport: " + transportReg.displayName);

    // Register a workflow
    sdk::WorkflowRegistration wfReg;
    wfReg.name = "acme-flash-workflow";
    wfReg.displayName = "ACME Flash Workflow";
    wfReg.description = "Standard ACME device flash workflow";
    wfReg.version = "1.0.0";
    wfReg.compatibleVendors = {"acme-corp"};
    wfReg.compatibleProtocols = {"acme-protocol"};
    wfReg.isDefault = true;
    wfReg.priority = 100;

    sdk->registerWorkflow(wfReg);
    logger->info("example", "Registered workflow: " + wfReg.displayName);

    // Register a job type
    sdk::JobRegistration jobReg;
    jobReg.name = "acme-flash-job";
    jobReg.displayName = "ACME Flash Job";
    jobReg.description = "Flash a partition on ACME devices";
    jobReg.version = "1.0.0";
    jobReg.jobType = "Flash";
    jobReg.compatibleVendors = {"acme-corp"};
    jobReg.compatibleProtocols = {"acme-protocol"};
    jobReg.supportsCancellation = true;
    jobReg.supportsProgress = true;
    jobReg.defaultPriority = 50;

    sdk->registerJob(jobReg);
    logger->info("example", "Registered job: " + jobReg.displayName);

    // Register a package format
    sdk::PackageRegistration pkgReg;
    pkgReg.name = "acme-package";
    pkgReg.displayName = "ACME Firmware Package";
    pkgReg.description = "ACME Corp encrypted firmware format";
    pkgReg.version = "1.0.0";
    pkgReg.supportedFileExtensions = {".acp", ".acme"};
    pkgReg.compatibleVendors = {discovery::Vendor::Custom};
    pkgReg.compatibleProtocols = {discovery::ProtocolType::Custom};
    pkgReg.requiresSignature = true;
    pkgReg.supportsCompression = true;

    sdk->registerPackage(pkgReg);
    logger->info("example", "Registered package format: " + pkgReg.displayName);

    // Register discovery method
    sdk::DiscoveryRegistration discReg;
    discReg.name = "acme-discovery";
    discReg.displayName = "ACME USB Discovery";
    discReg.description = "ACME device discovery over USB";
    discReg.version = "1.0.0";
    discReg.vendor = discovery::Vendor::Custom;
    discReg.supportedTransports = {discovery::TransportType::USB};
    discReg.usbVids = {0x1234};
    discReg.usbPids = {0x5678};
    discReg.serialBaudRates = {115200, 921600};
    discReg.scanTimeoutMs = 3000;

    sdk->registerDiscovery(discReg);
    logger->info("example", "Registered discovery: " + discReg.displayName);

    // Register capabilities
    sdk::CapabilityRegistration capReg;
    capReg.name = "acme-secure-flash";
    capReg.displayName = "ACME Secure Flash";
    capReg.description = "Authenticated flash capability";
    capReg.version = "1.0.0";
    capReg.capability = plugin::PluginCapability::Protocol;
    capReg.isRequired = false;
    capReg.priority = 200;

    sdk->registerCapability(capReg);
    logger->info("example", "Registered capability: " + capReg.displayName);

    // Finalize
    logger->info("example", "\nFinalizing SDK configuration...");
    auto report = sdk->finalize();

    logger->info("example", "SDK Report:");
    logger->info("example", "  Valid: " + std::string(report.valid ? "yes" : "no"));
    logger->info("example",
                 "  Vendors: " + std::to_string(report.registeredVendors.size()));
    logger->info("example",
                 "  Protocols: " + std::to_string(report.registeredProtocols.size()));
    logger->info("example",
                 "  Transports: " + std::to_string(report.registeredTransports.size()));
    logger->info("example",
                 "  Workflows: " + std::to_string(report.registeredWorkflows.size()));
    logger->info("example",
                 "  Jobs: " + std::to_string(report.registeredJobs.size()));
    logger->info("example",
                 "  Packages: " + std::to_string(report.registeredPackages.size()));
    logger->info("example",
                 "  Discoveries: " + std::to_string(report.registeredDiscoveries.size()));
    logger->info("example",
                 "  Capabilities: " + std::to_string(report.registeredCapabilities.size()));

    for (const auto& err : report.errors) {
        logger->error("example", "  Error: " + err);
    }
    for (const auto& warn : report.warnings) {
        logger->warn("example", "  Warning: " + warn);
    }

    if (report.valid) {
        logger->info("example",
                     "\nSDK configuration finalized successfully - ready for deployment.");
    } else {
        logger->error("example", "\nSDK configuration has errors - check output above.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
