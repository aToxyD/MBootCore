#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/PluginABI.hpp>

#include <string>
#include <cstdint>

using namespace mbootcore;
using namespace mbootcore::plugin;

class HelloPlugin final : public IPlugin {
public:
    HelloPlugin() = default;
    ~HelloPlugin() override = default;

    PluginMetadata metadata() const noexcept override {
        PluginMetadata meta;
        meta.name = "HelloPlugin";
        meta.version = "1.0.0";
        meta.apiVersion = PluginABIVersion;
        meta.author = "MBootCore";
        meta.description = "A minimal dynamic plugin example";
        meta.capabilities = PluginCapability::None;
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
        context.log("HelloPlugin initializing...");
        m_state = PluginState::Initialized;
        return {};
    }

    Result<void> shutdown() noexcept override {
        m_state = PluginState::Loaded;
        return {};
    }

    Result<void> registerComponents(PluginContext&) override {
        return {};
    }

    Result<void> unregisterComponents(PluginContext&) override {
        return {};
    }

private:
    PluginState m_state{PluginState::Unloaded};
    bool m_enabled{true};
};

extern "C" {

MBOOTCORE_PLUGIN_EXPORT IPlugin* mbootcore_plugin_create() {
    return new HelloPlugin();
}

MBOOTCORE_PLUGIN_EXPORT void mbootcore_plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

MBOOTCORE_PLUGIN_EXPORT uint32_t mbootcore_plugin_abi_version() {
    return PluginABIVersion;
}

}
