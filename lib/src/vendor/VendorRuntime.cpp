#include <mbootcore/vendor/VendorRuntime.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/vendor/IVendorPlugin.hpp>
#include <algorithm>

namespace mbootcore {
namespace vendor {

VendorRuntime::VendorRuntime() = default;
VendorRuntime::~VendorRuntime() { (void)shutdown(); }

Result<void> VendorRuntime::initialize(const VendorContext& context) {
    if (m_initialized) return ErrorCode::InvalidState;
    m_context = context;
    m_initialized = true;
    return {};
}

Result<void> VendorRuntime::shutdown() noexcept {
    if (!m_initialized) return {};
    auto vendors = m_registry.allVendors();
    std::vector<std::string> ids;
    ids.reserve(vendors.size());
    for (auto* v : vendors) ids.push_back(std::string(v->vendorInfo().id));
    for (auto* vendor : vendors) (void)vendor->shutdown();
    for (const auto& id : ids) (void)m_registry.unregisterVendor(id);
    m_initialized = false;
    m_activeVendorId.clear();
    return {};
}

Result<void> VendorRuntime::loadVendor(const std::string& vendorId) {
    if (!m_initialized) return ErrorCode::InvalidState;
    auto vendor = VendorFactory::createVendor(vendorId);
    if (!vendor) return ErrorCode::PluginNotFound;
    auto actualId = std::string(vendor->vendorInfo().id);
    auto result = vendor->initialize(m_context);
    if (!result.isOk()) return result;
    result = m_registry.registerVendor(std::move(vendor));
    if (!result.isOk()) return result;
    m_activeVendorId = actualId;
    return {};
}

Result<void> VendorRuntime::unloadVendor(const std::string& vendorId) {
    if (!m_initialized) return ErrorCode::InvalidState;
    auto result = m_registry.unregisterVendor(vendorId);
    if (m_activeVendorId == vendorId) m_activeVendorId.clear();
    return result;
}

Result<void> VendorRuntime::loadPlugin(std::unique_ptr<IVendorPlugin> plugin) {
    if (!m_initialized || !plugin) return ErrorCode::InvalidState;
    (void)plugin->registerVendor(m_context);
    return m_registry.registerPlugin(std::move(plugin));
}

Result<void> VendorRuntime::unloadPlugin(const std::string& pluginId) {
    return m_registry.unregisterPlugin(pluginId);
}

Result<void> VendorRuntime::createRuntimeContext(const std::string& vendorId) {
    (void)vendorId;
    return {};
}

IVendor* VendorRuntime::activeVendor() const noexcept {
    return m_registry.resolveById(m_activeVendorId);
}

VendorRegistry& VendorRuntime::registry() noexcept { return m_registry; }
const VendorRegistry& VendorRuntime::registry() const noexcept { return m_registry; }
const VendorContext& VendorRuntime::context() const noexcept { return m_context; }
VendorContext& VendorRuntime::mutableContext() noexcept { return m_context; }
bool VendorRuntime::isInitialized() const noexcept { return m_initialized; }

} // namespace vendor
} // namespace mbootcore
