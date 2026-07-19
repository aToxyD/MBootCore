#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace mbootcore {
namespace plugin {

enum class PluginState : uint32_t {
    Unloaded    = 0,
    Loaded      = 1,
    Initialized = 2,
    Enabled     = 3,
    Disabled    = 4,
    Error       = 5
};

enum class PluginCapability : uint32_t {
    None             = 0,
    Protocol         = 1 << 0,
    Vendor           = 1 << 1,
    Discovery        = 1 << 2,
    Negotiation      = 1 << 3,
    PipelineStage    = 1 << 4,
    RecoveryStrategy = 1 << 5,
    SessionExtension = 1 << 6,
    LoggingExtension = 1 << 7,
    LoaderMatcher    = 1 << 8,
    GPTExtension     = 1 << 9
};

inline PluginCapability operator|(PluginCapability a, PluginCapability b) noexcept {
    return static_cast<PluginCapability>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline PluginCapability operator&(PluginCapability a, PluginCapability b) noexcept {
    return static_cast<PluginCapability>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool hasCapability(PluginCapability caps, PluginCapability flag) noexcept {
    return (static_cast<uint32_t>(caps) & static_cast<uint32_t>(flag)) != 0;
}

struct PluginDependency {
    std::string pluginName;
    uint32_t    minVersion{1};
    uint32_t    maxVersion{UINT32_MAX};
    bool        optional{false};
};

struct PluginMetadata {
    std::string name;
    std::string version;
    uint32_t apiVersion{1};
    discovery::Vendor vendor{discovery::Vendor::Unknown};
    std::string author;
    std::string license;
    std::string description;
    std::vector<discovery::ProtocolType> supportedProtocols;
    PluginCapability capabilities{PluginCapability::None};
    PluginCapability requiredCapabilities{PluginCapability::None};
    uint32_t priority{100};
    std::string uuid;
    uint32_t compatibilityVersion{1};
    std::vector<PluginDependency> dependencies;
};

struct PluginConfig {
    bool enabled{true};
    uint32_t priority{100};
    bool autoLoad{true};
    bool compatibilityMode{false};
    std::unordered_map<std::string, std::string> vendorSettings;
};

} // namespace plugin
} // namespace mbootcore
