#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <unordered_map>

namespace mbootcore {
namespace vendor {

class IVendorPlugin;

class VendorRegistry {
public:
    VendorRegistry() = default;
    ~VendorRegistry() = default;

    VendorRegistry(const VendorRegistry&) = delete;
    VendorRegistry& operator=(const VendorRegistry&) = delete;
    VendorRegistry(VendorRegistry&&) = delete;
    VendorRegistry& operator=(VendorRegistry&&) = delete;

    Result<void> registerVendor(std::unique_ptr<IVendor> vendor);
    Result<void> registerPlugin(std::unique_ptr<IVendorPlugin> plugin);
    Result<void> unregisterVendor(const std::string& vendorId);
    Result<void> unregisterPlugin(const std::string& pluginId);
    void clear() noexcept;

    IVendor* resolveById(const std::string& vendorId) const;
    IVendor* resolveByFamily(VendorFamily family) const;
    IVendor* resolveByName(const std::string& name) const;
    std::vector<IVendor*> resolveByCapability(VendorCapability cap) const;
    std::vector<IVendor*> resolveByProtocol(const std::string& protocol) const;

    std::vector<IVendor*> allVendors() const;
    std::vector<IVendorPlugin*> allPlugins() const;
    size_t vendorCount() const noexcept;
    size_t pluginCount() const noexcept;
    bool isRegistered(const std::string& vendorId) const;

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::unique_ptr<IVendor>> m_vendors;
    std::unordered_map<std::string, std::unique_ptr<IVendorPlugin>> m_plugins;
};

} // namespace vendor
} // namespace mbootcore
