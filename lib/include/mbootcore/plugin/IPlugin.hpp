#pragma once

#include <mbootcore/plugin/PluginTypes.hpp>
#include <mbootcore/plugin/PluginContext.hpp>
#include <mbootcore/domain/Error.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace plugin {

struct VidPidEntry {
    uint16_t vid{0};
    uint16_t pid{0};
    std::string description;
};

struct ChipsetInfo {
    std::string name;
    std::vector<uint32_t> chipsetIds;
};

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual Result<void> initialize(PluginContext& context) = 0;
    virtual Result<void> shutdown() noexcept = 0;

    virtual PluginMetadata metadata() const noexcept = 0;

    virtual Result<void> registerComponents(PluginContext& context) = 0;
    virtual Result<void> unregisterComponents(PluginContext& context) = 0;

    virtual PluginState state() const noexcept = 0;
    virtual void setEnabled(bool enabled) noexcept = 0;
    virtual bool isEnabled() const noexcept = 0;

    virtual discovery::Vendor vendor() const noexcept { return discovery::Vendor::Unknown; }
    virtual std::vector<VidPidEntry> vidPidTable() const { return {}; }
    virtual std::vector<ChipsetInfo> knownChipsets() const { return {}; }
    virtual std::vector<discovery::BootMode> bootModes() const noexcept { return {}; }
};

} // namespace plugin
} // namespace mbootcore
