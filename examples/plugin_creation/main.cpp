#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/IProtocolPlugin.hpp>
#include <mbootcore/plugin/PluginTypes.hpp>
#include <mbootcore/plugin/PluginContext.hpp>
#include <mbootcore/plugin/PluginManager.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <iostream>
#include <cstdlib>
#include <memory>

using namespace mbootcore;

class DemoProtocolPlugin : public plugin::IProtocolPlugin {
public:
    DemoProtocolPlugin()
        : m_metadata{
              "DemoProtocol",
              "1.0.0",
              1,
              discovery::Vendor::Custom,
              "MBootCore Examples",
              "MIT",
              "Demonstration protocol plugin",
              {discovery::ProtocolType::Custom},
              plugin::PluginCapability::Protocol,
              {},
              100,
              "demo-protocol-0000-0000-000000000001",
              1,
              {}
          } {}

    Result<void> initialize(plugin::PluginContext& ctx) override {
        m_logger = ctx.logger();
        if (m_logger) {
            m_logger->info("DemoProtocolPlugin", "Initialized");
        }
        return {};
    }

    Result<void> shutdown() noexcept override {
        if (m_logger) {
            m_logger->info("DemoProtocolPlugin", "Shut down");
        }
        return {};
    }

    plugin::PluginMetadata metadata() const noexcept override {
        return m_metadata;
    }

    Result<void> registerComponents(plugin::PluginContext& ctx) override {
        (void)ctx;
        if (m_logger) {
            m_logger->info("DemoProtocolPlugin", "Registering components");
        }
        return {};
    }

    Result<void> unregisterComponents(plugin::PluginContext& ctx) override {
        (void)ctx;
        if (m_logger) {
            m_logger->info("DemoProtocolPlugin", "Unregistering components");
        }
        return {};
    }

    plugin::PluginState state() const noexcept override { return m_state; }
    void setEnabled(bool enabled) noexcept override { m_enabled = enabled; }
    bool isEnabled() const noexcept override { return m_enabled; }

    discovery::ProtocolType protocolType() const noexcept override {
        return discovery::ProtocolType::Custom;
    }

    std::vector<discovery::ProtocolType> supportedProtocols() const noexcept override {
        return {discovery::ProtocolType::Custom};
    }

private:
    plugin::PluginMetadata m_metadata;
    plugin::PluginState m_state{plugin::PluginState::Loaded};
    bool m_enabled{true};
    ILogger* m_logger{nullptr};
};

int main() {
    auto logger = std::make_shared<ConsoleLogger>();

    auto runtime = runtime::RuntimeFactory::createDefault();
    auto result = runtime.initialize();
    if (!result.isOk()) {
        std::cerr << "Init failed: " << toString(result.error()) << std::endl;
        return EXIT_FAILURE;
    }

    logger->info("example", "Runtime initialized");

    // List existing plugins
    auto existingPlugins = runtime.listPlugins();
    logger->info("example",
                 "Existing plugins: " + std::to_string(existingPlugins.size()));
    for (const auto& name : existingPlugins) {
        logger->info("example", "  - " + name);
    }

    // Create and install a custom plugin
    auto plugin = std::make_unique<DemoProtocolPlugin>();
    auto meta = plugin->metadata();
    logger->info("example",
                 "Installing plugin: " + meta.name + " v" + meta.version);

    auto installResult = runtime.installPlugin(std::move(plugin));
    if (installResult.isOk()) {
        logger->info("example", "Plugin installed successfully");
    } else {
        logger->error("example",
                      "Plugin install failed: " + std::string(toString(installResult.error())));
    }

    // List plugins again
    auto pluginsAfter = runtime.listPlugins();
    logger->info("example",
                 "Plugins after install: " + std::to_string(pluginsAfter.size()));
    for (const auto& name : pluginsAfter) {
        logger->info("example", "  - " + name);
    }

    // Attempt to remove the plugin
    auto removeResult = runtime.removePlugin("DemoProtocol");
    if (removeResult.isOk()) {
        logger->info("example", "Plugin removed successfully");
    } else {
        logger->info("example",
                     "Plugin removal returned: " + std::string(toString(removeResult.error())));
    }

    runtime.shutdown();
    logger->info("example", "Runtime shut down");
    return EXIT_SUCCESS;
}
