#include "mbootcore/plugin/PluginManager.hpp"
#include "platform/DynamicLibrary.hpp"

#include "SafeParser.hpp"

#include <algorithm>
#include <sstream>
#include <set>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace mbootcore {
namespace plugin {

PluginManager::PluginManager(PluginContext& context)
    : m_context(context) {}

PluginManager::~PluginManager() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [name, instance] : m_plugins) {
        if (instance.destroyFunc && instance.plugin) {
            auto* ptr = instance.plugin.release();
            instance.destroyFunc(ptr);
        }
    }
    m_plugins.clear();
}

Result<void> PluginManager::loadLocked(std::unique_ptr<IPlugin> plugin, PluginConfig config) {
    if (!plugin) {
        return ErrorCode::InvalidArgument;
    }

    auto meta = plugin->metadata();

    if (m_plugins.count(meta.name) > 0) {
        return ErrorCode::PluginDuplicate;
    }

    if (!isCompatible(meta)) {
        return ErrorCode::PluginIncompatible;
    }

    PluginInstance instance;
    instance.plugin = std::move(plugin);
    instance.config = std::move(config);
    instance.state = PluginState::Unloaded;
    m_plugins[meta.name] = std::move(instance);

    auto depResult = resolveDependenciesLocked(meta.name);
    if (depResult.isError()) {
        m_plugins.erase(meta.name);
        return depResult;
    }

    setPluginState(meta.name, PluginState::Loaded);

    m_context.log("Plugin loaded: " + meta.name);
    return {};
}

Result<void> PluginManager::load(std::unique_ptr<IPlugin> plugin, PluginConfig config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return loadLocked(std::move(plugin), std::move(config));
}

Result<void> PluginManager::loadFromFile(const std::string& filePath) {
    auto libResult = platform::DynamicLibrary::load(filePath);
    if (libResult.isError()) {
        return ErrorCode::PluginLoadFailed;
    }
    auto lib = std::move(libResult.value());

    auto versionSym = lib->symbol(PluginVersionSymbol);
    if (versionSym.isOk()) {
        auto versionFunc = reinterpret_cast<PluginVersionFunc>(versionSym.value());
        if (versionFunc() != PluginABIVersion) {
            return ErrorCode::PluginIncompatible;
        }
    }

    auto createSym = lib->symbol(PluginCreateSymbol);
    if (createSym.isError()) {
        return ErrorCode::PluginLoadFailed;
    }
    auto createFunc = reinterpret_cast<PluginCreateFunc>(createSym.value());

    auto destroySym = lib->symbol(PluginDestroySymbol);
    if (destroySym.isError()) {
        return ErrorCode::PluginLoadFailed;
    }
    auto destroyFunc = reinterpret_cast<PluginDestroyFunc>(destroySym.value());

    IPlugin* rawPlugin = createFunc();
    if (!rawPlugin) {
        return ErrorCode::PluginLoadFailed;
    }

    auto meta = rawPlugin->metadata();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_plugins.count(meta.name) > 0) {
            destroyFunc(rawPlugin);
            return ErrorCode::PluginDuplicate;
        }

        if (!isCompatible(meta)) {
            destroyFunc(rawPlugin);
            return ErrorCode::PluginIncompatible;
        }

        PluginInstance instance;
        instance.plugin.reset(rawPlugin);
        instance.library = std::move(lib);
        instance.destroyFunc = destroyFunc;
        instance.state = PluginState::Unloaded;
        m_plugins[meta.name] = std::move(instance);

        auto depResult = resolveDependenciesLocked(meta.name);
        if (depResult.isError()) {
            m_plugins.erase(meta.name);
            return depResult;
        }

        setPluginState(meta.name, PluginState::Loaded);
    }

    m_context.log("Plugin loaded from file: " + meta.name + " [" + filePath + "]");
    return {};
}

Result<void> PluginManager::loadAllFromDirectory(const std::string& directoryPath) {
#ifdef _WIN32
    std::string pattern = directoryPath + "\\*.dll";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return {};
    }
    do {
        std::string fullPath = directoryPath + "\\" + findData.cFileName;
        auto result = loadFromFile(fullPath);
        if (result.isError()) {
            m_context.log("Failed to load plugin: " + fullPath);
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
#else
    DIR* dir = opendir(directoryPath.c_str());
    if (!dir) {
        return {};
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name.size() < 3) continue;
        auto ext = name.substr(name.size() - 3);
        if (ext != ".so") continue;
        std::string fullPath = directoryPath + "/" + name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        auto result = loadFromFile(fullPath);
        if (result.isError()) {
            m_context.log("Failed to load plugin: " + fullPath);
        }
    }
    closedir(dir);
#endif
    return {};
}

Result<void> PluginManager::unload(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }

    auto& instance = it->second;
    if (instance.state >= PluginState::Initialized) {
        auto shutdownResult = shutdownLocked(name);
        if (shutdownResult.isError()) {
            return shutdownResult;
        }
    }

    auto meta = instance.plugin->metadata();
    auto unregResult = unregisterPluginComponents(*instance.plugin, meta);
    if (unregResult.isError()) {
        return unregResult;
    }

    if (instance.destroyFunc && instance.plugin) {
        auto* ptr = instance.plugin.release();
        instance.destroyFunc(ptr);
    }
    m_plugins.erase(it);
    m_context.log("Plugin unloaded: " + name);
    return {};
}

Result<void> PluginManager::initializeLocked(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }

    auto& instance = it->second;
    if (instance.state >= PluginState::Initialized) {
        return {};
    }
    if (instance.state == PluginState::Error) {
        return ErrorCode::PluginInitFailed;
    }

    auto meta = instance.plugin->metadata();

    auto regResult = registerPluginComponents(*instance.plugin, meta);
    if (regResult.isError()) {
        setPluginState(name, PluginState::Error);
        return regResult;
    }

    auto initResult = instance.plugin->initialize(m_context);
    if (initResult.isError()) {
        (void)unregisterPluginComponents(*instance.plugin, meta);
        setPluginState(name, PluginState::Error);
        return initResult;
    }

    setPluginState(name, PluginState::Initialized);

    if (instance.config.enabled) {
        setPluginState(name, PluginState::Enabled);
    }

    m_context.log("Plugin initialized: " + name);
    return {};
}

Result<void> PluginManager::initialize(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return initializeLocked(name);
}

Result<void> PluginManager::shutdownLocked(const std::string& name) noexcept {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }

    auto& instance = it->second;
    if (instance.state < PluginState::Initialized) {
        return {};
    }

    instance.state = PluginState::Disabled;

    auto result = instance.plugin->shutdown();
    if (result.isError()) {
        setPluginState(name, PluginState::Error);
        return result;
    }

    auto meta = instance.plugin->metadata();
    (void)unregisterPluginComponents(*instance.plugin, meta);

    setPluginState(name, PluginState::Loaded);
    m_context.log("Plugin shut down: " + name);
    return {};
}

Result<void> PluginManager::shutdown(const std::string& name) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return shutdownLocked(name);
}

Result<void> PluginManager::enable(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }
    it->second.plugin->setEnabled(true);
    it->second.config.enabled = true;
    if (it->second.state >= PluginState::Initialized) {
        setPluginState(name, PluginState::Enabled);
    }
    return {};
}

Result<void> PluginManager::disable(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }
    it->second.plugin->setEnabled(false);
    it->second.config.enabled = false;
    if (it->second.state >= PluginState::Initialized) {
        setPluginState(name, PluginState::Disabled);
    }
    return {};
}

Result<void> PluginManager::initializeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [name, instance] : m_plugins) {
        if (instance.state == PluginState::Loaded) {
            auto result = initializeLocked(name);
            if (result.isError()) {
                return result;
            }
        }
    }
    return {};
}

Result<void> PluginManager::shutdownAll() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [name, instance] : m_plugins) {
        if (instance.state >= PluginState::Initialized) {
            (void)shutdownLocked(name);
        }
    }
    return {};
}

Result<void> PluginManager::loadAll(std::vector<std::unique_ptr<IPlugin>> plugins) {
    for (auto& plugin : plugins) {
        if (!plugin) continue;
        auto meta = plugin->metadata();
        PluginConfig config;
        config.enabled = true;
        auto result = load(std::move(plugin), config);
        if (result.isError()) {
            return result;
        }
    }
    return {};
}

IPlugin* PluginManager::findPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        return it->second.plugin.get();
    }
    return nullptr;
}

IProtocolPlugin* PluginManager::findProtocolPlugin(discovery::ProtocolType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [name, instance] : m_plugins) {
        auto* protoPlugin = dynamic_cast<IProtocolPlugin*>(instance.plugin.get());
        if (protoPlugin && protoPlugin->protocolType() == type) {
            return protoPlugin;
        }
    }
    return nullptr;
}

IPlugin* PluginManager::findVendorPlugin(discovery::Vendor vendor) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [name, instance] : m_plugins) {
        auto* plugin = instance.plugin.get();
        if (plugin && plugin->vendor() == vendor) {
            return plugin;
        }
    }
    return nullptr;
}

PluginState PluginManager::pluginState(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        return it->second.state;
    }
    return PluginState::Unloaded;
}

const PluginInstance* PluginManager::pluginInfo(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> PluginManager::listPlugins() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    names.reserve(m_plugins.size());
    for (const auto& [name, instance] : m_plugins) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::string> PluginManager::listPluginsByState(PluginState state) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    for (const auto& [name, instance] : m_plugins) {
        if (instance.state == state) {
            names.push_back(name);
        }
    }
    return names;
}

std::vector<std::string> PluginManager::listEnabledPlugins() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    for (const auto& [name, instance] : m_plugins) {
        if (instance.config.enabled && instance.plugin->isEnabled()) {
            names.push_back(name);
        }
    }
    return names;
}

Result<void> PluginManager::reload(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }

    auto plugin = std::move(it->second.plugin);
    auto config = it->second.config;
    auto meta = plugin->metadata();

    (void)unregisterPluginComponents(*plugin, meta);
    m_plugins.erase(it);

    auto loadResult = loadLocked(std::move(plugin), config);
    if (loadResult.isError()) {
        return loadResult;
    }

    return initializeLocked(name);
}

Result<void> PluginManager::resolveDependenciesLocked(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return ErrorCode::PluginNotFound;
    }

    auto meta = it->second.plugin->metadata();

    std::unordered_set<std::string> visited;
    if (hasCircularDependencyLocked(name, visited)) {
        return ErrorCode::PluginCircularDependency;
    }

    for (const auto& dep : meta.dependencies) {
        auto depIt = m_plugins.find(dep.pluginName);
        if (depIt == m_plugins.end()) {
            if (!dep.optional) {
                return ErrorCode::PluginDependencyMissing;
            }
            continue;
        }

        auto depMeta = depIt->second.plugin->metadata();
        uint32_t depVersion = 1;
        auto parseResult = fromCharsUint32(depMeta.version);
        if (parseResult.ok) depVersion = parseResult.value;

        if (depVersion < dep.minVersion || depVersion > dep.maxVersion) {
            return ErrorCode::PluginVersionMismatch;
        }
    }

    return {};
}

Result<void> PluginManager::resolveDependencies(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return resolveDependenciesLocked(name);
}

bool PluginManager::hasCircularDependencyLocked(const std::string& name,
                                           std::unordered_set<std::string>& visited) const {
    if (visited.count(name)) return true;
    visited.insert(name);

    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) return false;

    auto meta = it->second.plugin->metadata();
    for (const auto& dep : meta.dependencies) {
        if (m_plugins.count(dep.pluginName)) {
            if (hasCircularDependencyLocked(dep.pluginName, visited)) return true;
        }
    }

    visited.erase(name);
    return false;
}

bool PluginManager::hasCircularDependency(const std::string& name,
                                           std::unordered_set<std::string>& visited) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return hasCircularDependencyLocked(name, visited);
}

std::size_t PluginManager::pluginCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_plugins.size();
}

bool PluginManager::isCompatible(const PluginMetadata& metadata) const {
    return metadata.compatibilityVersion >= 1 && metadata.compatibilityVersion <= 2;
}

Result<void> PluginManager::registerPluginComponents(IPlugin& plugin, const PluginMetadata& meta) {
    (void)meta;
    return plugin.registerComponents(m_context);
}

Result<void> PluginManager::unregisterPluginComponents(IPlugin& plugin, const PluginMetadata& meta) {
    (void)meta;
    return plugin.unregisterComponents(m_context);
}

void PluginManager::setPluginState(const std::string& name, PluginState state) {
    // Caller must hold m_mutex
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        it->second.state = state;
    }
}

} // namespace plugin
} // namespace mbootcore
