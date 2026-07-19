#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct PluginMetadataExtended {
    std::string name;
    std::string version;
    std::string sdkVersion;
    std::string vendor;
    std::string author;
    std::string license;
    std::string description;
    std::string homeUrl;
    std::string supportContact;
    std::string minRuntimeVersion;
    std::string maxRuntimeVersion;
    std::vector<std::string> capabilities;
    std::vector<std::string> supportedProtocols;
    std::vector<std::string> supportedVendors;
    std::vector<std::string> dependencies;
    std::vector<std::string> optionalDependencies;
    std::unordered_map<std::string, std::string> customAttributes;
    uint32_t compatibilityVersion{1};
    uint64_t sizeBytes{0};
    bool isSigned{false};
    bool isSystemPlugin{false};
    bool isBuiltin{false};
};

} // namespace sdk
} // namespace mbootcore
