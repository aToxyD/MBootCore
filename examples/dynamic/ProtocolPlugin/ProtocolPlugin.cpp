#include <mbootcore/plugin/IProtocolPlugin.hpp>
#include <mbootcore/plugin/PluginABI.hpp>

#include <string>
#include <vector>
#include <cstdint>

using namespace mbootcore;
using namespace mbootcore::plugin;

class DynamicProtocolPlugin final : public IProtocolPlugin {
public:
    DynamicProtocolPlugin() = default;
    ~DynamicProtocolPlugin() override = default;

    PluginMetadata metadata() const noexcept override {
        PluginMetadata meta;
        meta.name = "DynamicProtocolPlugin";
        meta.version = "1.0.0";
        meta.apiVersion = PluginABIVersion;
        meta.vendor = discovery::Vendor::Custom;
        meta.author = "MBootCore";
        meta.description = "A dynamic protocol plugin loaded from .so";
        meta.capabilities = PluginCapability::Protocol;
        meta.supportedProtocols = { discovery::ProtocolType::Custom };
        meta.requiredCapabilities = PluginCapability::Protocol;
        meta.priority = 100;
        meta.compatibilityVersion = 1;
        return meta;
    }

    PluginState state() const noexcept override {
        return m_state;
    }

    void setEnabled(bool enabled) noexcept override {
        m_enabled = enabled;
    }

    bool isEnabled() const noexcept override {
        return m_enabled;
    }

    Result<void> initialize(PluginContext& context) override {
        context.log("DynamicProtocolPlugin initializing...");
        m_state = PluginState::Initialized;
        return {};
    }

    Result<void> shutdown() noexcept override {
        m_state = PluginState::Loaded;
        return {};
    }

    Result<void> registerComponents(PluginContext& context) override {
        context.log("DynamicProtocolPlugin registering protocol components");
        return {};
    }

    Result<void> unregisterComponents(PluginContext& context) override {
        context.log("DynamicProtocolPlugin unregistering protocol components");
        return {};
    }

    discovery::ProtocolType protocolType() const noexcept override {
        return discovery::ProtocolType::Custom;
    }

    std::vector<discovery::ProtocolType> supportedProtocols() const noexcept override {
        return { discovery::ProtocolType::Custom };
    }

private:
    PluginState m_state{PluginState::Unloaded};
    bool m_enabled{true};
};

extern "C" {

MBOOTCORE_PLUGIN_EXPORT IPlugin* mbootcore_plugin_create() {
    return new DynamicProtocolPlugin();
}

MBOOTCORE_PLUGIN_EXPORT void mbootcore_plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

MBOOTCORE_PLUGIN_EXPORT uint32_t mbootcore_plugin_abi_version() {
    return PluginABIVersion;
}

}
