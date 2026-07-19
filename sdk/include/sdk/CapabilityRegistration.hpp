#pragma once

#include <mbootcore/plugin/PluginTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct CapabilityRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    plugin::PluginCapability capability{plugin::PluginCapability::None};
    std::vector<plugin::PluginCapability> requiredCapabilities;
    std::vector<plugin::PluginCapability> optionalCapabilities;
    std::vector<std::string> dependencies;
    bool isRequired{false};
    uint32_t priority{100};
};

} // namespace sdk
} // namespace mbootcore
