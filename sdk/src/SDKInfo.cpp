#include <sdk/SDKInfo.hpp>
#include <algorithm>
#include <sstream>

namespace mbootcore {
namespace sdk {

SDKInfo SDKInfo::collect() {
    SDKInfo info;

    info.versionInfo.sdkVersion = SemanticVersion{1, 0, 0, "", ""};
    info.versionInfo.coreVersion = SemanticVersion{1, 0, 0, "", ""};

    info.components = {
        {"mbootcore", "1.0.0", "available", "Core BootROM protocol framework"},
        {"mbootsdk", "1.0.0", "available", "SDK for vendor plugin development"},
        {"plugins", "1.0.0", "available", "Vendor plugin system"},
        {"cli", "1.0.0", "available", "Command-line interface"},
        {"examples", "1.0.0", "available", "Example applications"}
    };

    info.capabilities = {
        {"device_discovery", "Discover connected devices", true},
        {"protocol_negotiation", "Negotiate boot protocols", true},
        {"firmware_flashing", "Flash firmware to devices", true},
        {"sahara_protocol", "Qualcomm Sahara protocol support", true},
        {"firehose_protocol", "Qualcomm Firehose protocol support", true},
        {"gpt_partitioning", "GPT partition management", true},
        {"pipeline_execution", "Boot pipeline orchestration", true},
        {"plugin_loading", "Dynamic plugin loading", true},
        {"job_scheduling", "Job scheduling and execution", true},
        {"firmware_packaging", "Firmware package creation", true},
        {"elf_loading", "ELF binary loading", true},
        {"device_detection", "USB/Serial device detection", true}
    };

    info.supportedPlatforms = {"Windows", "Linux", "macOS"};

    info.availableTransports = {"USB", "Serial", "TCP"};

    info.availableProtocols = {"Sahara", "Firehose"};

    info.availableFeatures = {
        "Hotplug support",
        "Multi-device management",
        "Stress testing",
        "Virtual device simulation",
        "Fault injection testing",
        "Session recovery",
        "Plugin dependency resolution",
        "SDK template generation"
    };

    return info;
}

std::string SDKInfo::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"sdkVersion\": \"" << versionInfo.sdkVersion.toString() << "\",\n";
    ss << "  \"major\": " << versionInfo.sdkVersion.major << ",\n";
    ss << "  \"minor\": " << versionInfo.sdkVersion.minor << ",\n";
    ss << "  \"patch\": " << versionInfo.sdkVersion.patch << ",\n";

    ss << "  \"components\": [\n";
    for (size_t i = 0; i < components.size(); ++i) {
        const auto& c = components[i];
        ss << "    {\"name\": \"" << c.name << "\", \"version\": \"" << c.version
           << "\", \"status\": \"" << c.status << "\", \"detail\": \"" << c.detail << "\"}";
        if (i + 1 < components.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"capabilities\": [\n";
    for (size_t i = 0; i < capabilities.size(); ++i) {
        const auto& c = capabilities[i];
        ss << "    {\"name\": \"" << c.name << "\", \"description\": \"" << c.description
           << "\", \"available\": " << (c.available ? "true" : "false") << "}";
        if (i + 1 < capabilities.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"supportedPlatforms\": [";
    for (size_t i = 0; i < supportedPlatforms.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << supportedPlatforms[i] << "\"";
    }
    ss << "],\n";

    ss << "  \"availableTransports\": [";
    for (size_t i = 0; i < availableTransports.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << availableTransports[i] << "\"";
    }
    ss << "],\n";

    ss << "  \"availableProtocols\": [";
    for (size_t i = 0; i < availableProtocols.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << availableProtocols[i] << "\"";
    }
    ss << "],\n";

    ss << "  \"availableFeatures\": [";
    for (size_t i = 0; i < availableFeatures.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << availableFeatures[i] << "\"";
    }
    ss << "]\n";
    ss << "}";
    return ss.str();
}

std::string SDKInfo::toString() const {
    std::ostringstream ss;
    ss << "MBootCore SDK Information\n";
    ss << "========================\n\n";
    ss << "SDK Version: " << versionInfo.sdkVersion.toString() << " ("
       << versionInfo.sdkVersion.major << "." << versionInfo.sdkVersion.minor
       << "." << versionInfo.sdkVersion.patch << ")\n\n";

    ss << "Components:\n";
    for (const auto& c : components) {
        ss << "  - " << c.name << " v" << c.version
           << " [" << c.status << "]: " << c.detail << "\n";
    }
    ss << "\n";

    ss << "Capabilities:\n";
    for (const auto& c : capabilities) {
        ss << "  - " << c.name << ": " << c.description
           << " (" << (c.available ? "available" : "unavailable") << ")\n";
    }
    ss << "\n";

    ss << "Supported Platforms: ";
    for (size_t i = 0; i < supportedPlatforms.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << supportedPlatforms[i];
    }
    ss << "\n";

    ss << "Available Transports: ";
    for (size_t i = 0; i < availableTransports.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << availableTransports[i];
    }
    ss << "\n";

    ss << "Available Protocols: ";
    for (size_t i = 0; i < availableProtocols.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << availableProtocols[i];
    }
    ss << "\n";

    ss << "Available Features:\n";
    for (const auto& f : availableFeatures) {
        ss << "  - " << f << "\n";
    }

    return ss.str();
}

} // namespace sdk
} // namespace mbootcore
