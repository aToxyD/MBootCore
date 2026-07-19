#pragma once

#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/IProtocolPlugin.hpp>
#include <mbootcore/plugin/PluginContext.hpp>
#include <mbootcore/plugin/PluginABI.hpp>

#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mbootcore {
namespace platform {
class DynamicLibrary;
} // namespace platform

namespace plugin {

struct PluginInstance {
    std::unique_ptr<IPlugin> plugin;
    PluginConfig config;
    PluginState state{PluginState::Unloaded};
    std::unique_ptr<platform::DynamicLibrary> library;
    PluginDestroyFunc destroyFunc{nullptr};
};

class PluginManager {
public:
    explicit PluginManager(PluginContext& context);

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;
    ~PluginManager();

    // Lifecycle
    Result<void> load(std::unique_ptr<IPlugin> plugin, PluginConfig config = PluginConfig{});
    Result<void> loadFromFile(const std::string& filePath);
    Result<void> unload(const std::string& name);
    Result<void> initialize(const std::string& name);
    Result<void> shutdown(const std::string& name) noexcept;
    Result<void> enable(const std::string& name);
    Result<void> disable(const std::string& name);

    // Batch
    Result<void> initializeAll();
    Result<void> shutdownAll() noexcept;
    Result<void> loadAll(std::vector<std::unique_ptr<IPlugin>> plugins);
    Result<void> loadAllFromDirectory(const std::string& directoryPath);

    // Query
    IPlugin* findPlugin(const std::string& name) const;
    IProtocolPlugin* findProtocolPlugin(discovery::ProtocolType type) const;
    IPlugin* findVendorPlugin(discovery::Vendor vendor) const;

    PluginState pluginState(const std::string& name) const;
    const PluginInstance* pluginInfo(const std::string& name) const;

    std::vector<std::string> listPlugins() const;
    std::vector<std::string> listPluginsByState(PluginState state) const;
    std::vector<std::string> listEnabledPlugins() const;

    std::size_t pluginCount() const noexcept;

    // Reload
    Result<void> reload(const std::string& name);

    // Resolution
    Result<void> resolveDependencies(const std::string& name);
    bool hasCircularDependency(const std::string& name, std::unordered_set<std::string>& visited) const;
    bool isCompatible(const PluginMetadata& metadata) const;

    PluginContext& context() noexcept { return m_context; }

private:
    Result<void> registerPluginComponents(IPlugin& plugin, const PluginMetadata& meta);
    Result<void> unregisterPluginComponents(IPlugin& plugin, const PluginMetadata& meta);

    void setPluginState(const std::string& name, PluginState state);

    mutable std::mutex m_mutex;

    // Locked helpers (caller must hold m_mutex)
    Result<void> loadLocked(std::unique_ptr<IPlugin> plugin, PluginConfig config);
    Result<void> initializeLocked(const std::string& name);
    Result<void> shutdownLocked(const std::string& name) noexcept;
    Result<void> resolveDependenciesLocked(const std::string& name);
    bool hasCircularDependencyLocked(const std::string& name, std::unordered_set<std::string>& visited) const;

    PluginContext& m_context;
    std::unordered_map<std::string, PluginInstance> m_plugins;
    std::unordered_map<std::string, std::vector<std::string>> m_dependencyGraph;
};

} // namespace plugin
} // namespace mbootcore
