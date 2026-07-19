#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/PluginABI.hpp>

#include <string>
#include <cstdint>

using namespace mbootcore;
using namespace mbootcore::plugin;

class TestDynamicPlugin final : public IPlugin {
public:
    TestDynamicPlugin() = default;
    ~TestDynamicPlugin() override = default;

    PluginMetadata metadata() const noexcept override {
        PluginMetadata meta;
        meta.name = "TestDynamicPlugin";
        meta.version = "1.0.0";
        meta.apiVersion = PluginABIVersion;
        meta.author = "test";
        meta.description = "Test plugin for dynamic loading tests";
        meta.capabilities = PluginCapability::None;
        meta.priority = 100;
        meta.compatibilityVersion = 1;
        return meta;
    }

    PluginState state() const noexcept override { return m_state; }
    void setEnabled(bool enabled) noexcept override { m_enabled = enabled; }
    bool isEnabled() const noexcept override { return m_enabled; }

    Result<void> initialize(PluginContext&) override {
        m_initCount++;
        m_state = PluginState::Initialized;
        return {};
    }

    Result<void> shutdown() noexcept override {
        m_shutdownCount++;
        m_state = PluginState::Loaded;
        return {};
    }

    Result<void> registerComponents(PluginContext&) override {
        m_regCount++;
        return {};
    }

    Result<void> unregisterComponents(PluginContext&) override {
        m_unregCount++;
        return {};
    }

    int initCount() const noexcept { return m_initCount; }
    int shutdownCount() const noexcept { return m_shutdownCount; }
    int regCount() const noexcept { return m_regCount; }
    int unregCount() const noexcept { return m_unregCount; }

private:
    PluginState m_state{PluginState::Unloaded};
    bool m_enabled{true};
    int m_initCount{0};
    int m_shutdownCount{0};
    int m_regCount{0};
    int m_unregCount{0};
};

extern "C" {

MBOOTCORE_PLUGIN_EXPORT IPlugin* mbootcore_plugin_create() {
    return new TestDynamicPlugin();
}

MBOOTCORE_PLUGIN_EXPORT void mbootcore_plugin_destroy(IPlugin* plugin) {
    delete plugin;
}

MBOOTCORE_PLUGIN_EXPORT uint32_t mbootcore_plugin_abi_version() {
    return PluginABIVersion;
}

}
