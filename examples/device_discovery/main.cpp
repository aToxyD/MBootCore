#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/IDeviceService.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>

static const char* vendorToString(mbootcore::discovery::Vendor v) {
    using namespace mbootcore::discovery;
    switch (v) {
        case Vendor::Qualcomm:   return "Qualcomm";
        case Vendor::MediaTek:   return "MediaTek";
        case Vendor::UNISOC:     return "UNISOC";
        case Vendor::Samsung:    return "Samsung";
        case Vendor::Rockchip:   return "Rockchip";
        case Vendor::Spreadtrum: return "Spreadtrum";
        case Vendor::Apple:      return "Apple";
        case Vendor::Google:     return "Google";
        case Vendor::Huawei:     return "Huawei";
        case Vendor::Custom:     return "Custom";
        default:                 return "Unknown";
    }
}

static const char* protocolToString(mbootcore::discovery::ProtocolType p) {
    using namespace mbootcore::discovery;
    switch (p) {
        case ProtocolType::Sahara:        return "Sahara";
        case ProtocolType::Firehose:      return "Firehose";
        case ProtocolType::Fastboot:      return "Fastboot";
        case ProtocolType::MediaTekBROM:  return "MediaTekBROM";
        case ProtocolType::MediaTekDA:    return "MediaTekDA";
        case ProtocolType::UNISOCBootROM: return "UNISOCBootROM";
        case ProtocolType::UNISOCFDL:     return "UNISOCFDL";
        default:                          return "Unknown";
    }
}

static const char* transportToString(mbootcore::discovery::TransportType t) {
    using namespace mbootcore::discovery;
    switch (t) {
        case TransportType::USB:    return "USB";
        case TransportType::Serial: return "Serial";
        case TransportType::TCP:    return "TCP";
        case TransportType::Virtual: return "Virtual";
        default:                    return "Unknown";
    }
}

int main() {
    using namespace mbootcore;

    auto logger = std::make_shared<ConsoleLogger>();

    auto runtime = runtime::RuntimeFactory::createDefault();
    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Init failed: " << toString(result.error()) << std::endl;
        return EXIT_FAILURE;
    }

    auto& deviceService = runtime.services().devices();

    // Discover all devices
    logger->info("example", "Discovering all devices...");
    auto discoverResult = deviceService.discover(std::chrono::seconds(5));
    if (!discoverResult.isOk()) {
        logger->error("example", "Discovery failed");
        runtime.shutdown();
        return EXIT_FAILURE;
    }

    const auto& devices = discoverResult.value();
    logger->info("example",
                 "Found " + std::to_string(devices.size()) + " device(s)");

    for (const auto& d : devices) {
        auto output = "  Device: " + d.friendlyName +
                      "\n    Vendor: " + vendorToString(d.vendor) +
                      "\n    Protocol: " + protocolToString(d.protocolHint) +
                      "\n    Transport: " + transportToString(d.transport) +
                      "\n    Boot Mode: " + std::to_string(static_cast<int>(d.bootMode));

        if (d.isUsb()) {
            output += "\n    USB VID:PID = " +
                      std::to_string(d.usbVid) + ":" + std::to_string(d.usbPid);
        }
        if (d.isSerial()) {
            output += "\n    Serial Port: " + d.serialPort +
                      " @ " + std::to_string(d.serialBaudRate) + " baud";
        }
        if (d.isTcp()) {
            output += "\n    TCP: " + d.tcpHost + ":" + std::to_string(d.tcpPort);
        }

        logger->info("example", output);

        if (!d.capabilities.empty()) {
            for (const auto& [k, v] : d.capabilities) {
                logger->info("example",
                             "    [cap] " + k + " = " + v);
            }
        }
    }

    // Discover by vendor (Qualcomm as example)
    logger->info("example", "\nSearching for Qualcomm devices...");
    std::vector<discovery::DeviceDescriptor> qcDevices;
    std::copy_if(devices.begin(), devices.end(), std::back_inserter(qcDevices),
        [](const discovery::DeviceDescriptor& d) {
            return d.vendor == discovery::Vendor::Qualcomm;
        });
    logger->info("example",
                 "Qualcomm devices: " + std::to_string(qcDevices.size()));

    // Try probing a USB hint
    discovery::DeviceDescriptor hint;
    hint.transport = discovery::TransportType::USB;
    hint.usbVid = 0x05C6;
    hint.usbPid = 0x9008;
    hint.vendor = discovery::Vendor::Qualcomm;
    hint.bootMode = discovery::BootMode::EDL;

    logger->info("example", "\nProbing for EDL device (VID=05C6, PID=9008)...");
    auto probeResult = deviceService.probe(hint);
    if (probeResult.isOk()) {
        auto& dev = probeResult.value();
        logger->info("example",
                     "Probe succeeded: " + dev.friendlyName +
                         " protocol=" + protocolToString(dev.protocolHint));
    } else {
        logger->info("example",
                     "Probe returned: " + std::string(toString(probeResult.error())));
    }

    runtime.shutdown();
    return EXIT_SUCCESS;
}
