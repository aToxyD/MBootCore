#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/vendor/IVendorPlugin.hpp>
#include <algorithm>
#include <shared_mutex>

namespace mbootcore {
namespace vendor {

Result<void> VendorRegistry::registerVendor(std::unique_ptr<IVendor> vendor) {
    if (!vendor) return ErrorCode::InvalidArgument;
    std::unique_lock lock(m_mutex);
    auto id = std::string(vendor->vendorInfo().id);
    if (m_vendors.find(id) != m_vendors.end()) {
        return ErrorCode::AlreadyExists;
    }
    m_vendors[std::move(id)] = std::move(vendor);
    return {};
}

Result<void> VendorRegistry::registerPlugin(std::unique_ptr<IVendorPlugin> plugin) {
    if (!plugin) return ErrorCode::InvalidArgument;
    std::unique_lock lock(m_mutex);
    auto id = std::string(plugin->name());
    if (m_plugins.find(id) != m_plugins.end()) {
        return ErrorCode::AlreadyExists;
    }
    m_plugins[std::move(id)] = std::move(plugin);
    return {};
}

Result<void> VendorRegistry::unregisterVendor(const std::string& vendorId) {
    std::unique_ptr<IVendor> vendor;
    {
        std::unique_lock lock(m_mutex);
        auto it = m_vendors.find(vendorId);
        if (it == m_vendors.end()) return ErrorCode::PluginNotFound;
        vendor = std::move(it->second);
        m_vendors.erase(it);
    }
    return vendor->shutdown();
}

Result<void> VendorRegistry::unregisterPlugin(const std::string& pluginId) {
    std::unique_ptr<IVendorPlugin> plugin;
    {
        std::unique_lock lock(m_mutex);
        auto it = m_plugins.find(pluginId);
        if (it == m_plugins.end()) return ErrorCode::PluginNotFound;
        plugin = std::move(it->second);
        m_plugins.erase(it);
    }
    return plugin->unregisterVendor();
}

IVendor* VendorRegistry::resolveById(const std::string& vendorId) const {
    std::shared_lock lock(m_mutex);
    auto it = m_vendors.find(vendorId);
    return it != m_vendors.end() ? it->second.get() : nullptr;
}

IVendor* VendorRegistry::resolveByFamily(VendorFamily family) const {
    std::shared_lock lock(m_mutex);
    for (const auto& [id, vendor] : m_vendors) {
        if (vendor->vendorInfo().family == family) return vendor.get();
    }
    return nullptr;
}

IVendor* VendorRegistry::resolveByName(const std::string& name) const {
    std::shared_lock lock(m_mutex);
    for (const auto& [id, vendor] : m_vendors) {
        if (vendor->vendorInfo().name == name) return vendor.get();
    }
    return nullptr;
}

std::vector<IVendor*> VendorRegistry::resolveByCapability(VendorCapability cap) const {
    std::shared_lock lock(m_mutex);
    std::vector<IVendor*> result;
    for (const auto& [id, vendor] : m_vendors) {
        if (hasCapability(vendor->capabilities(), cap)) {
            result.push_back(vendor.get());
        }
    }
    return result;
}

std::vector<IVendor*> VendorRegistry::resolveByProtocol(const std::string& protocol) const {
    std::shared_lock lock(m_mutex);
    std::vector<IVendor*> result;
    for (const auto& [id, vendor] : m_vendors) {
        const auto& protocols = vendor->vendorInfo().supportedProtocols;
        if (std::find(protocols.begin(), protocols.end(), protocol) != protocols.end()) {
            result.push_back(vendor.get());
        }
    }
    return result;
}

std::vector<IVendor*> VendorRegistry::allVendors() const {
    std::shared_lock lock(m_mutex);
    std::vector<IVendor*> result;
    result.reserve(m_vendors.size());
    for (const auto& [id, vendor] : m_vendors) result.push_back(vendor.get());
    return result;
}

std::vector<IVendorPlugin*> VendorRegistry::allPlugins() const {
    std::shared_lock lock(m_mutex);
    std::vector<IVendorPlugin*> result;
    result.reserve(m_plugins.size());
    for (const auto& [id, plugin] : m_plugins) result.push_back(plugin.get());
    return result;
}

size_t VendorRegistry::vendorCount() const noexcept {
    std::shared_lock lock(m_mutex);
    return m_vendors.size();
}
size_t VendorRegistry::pluginCount() const noexcept {
    std::shared_lock lock(m_mutex);
    return m_plugins.size();
}

bool VendorRegistry::isRegistered(const std::string& vendorId) const {
    std::shared_lock lock(m_mutex);
    return m_vendors.find(vendorId) != m_vendors.end();
}

void VendorRegistry::clear() noexcept {
    std::unique_lock lock(m_mutex);
    m_vendors.clear();
    m_plugins.clear();
}

} // namespace vendor
} // namespace mbootcore
