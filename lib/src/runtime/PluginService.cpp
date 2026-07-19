#include "PluginService.hpp"

namespace mbootcore {
namespace runtime {

PluginService::PluginService(
    std::unique_ptr<plugin::PluginManager> pluginManager,
    std::unique_ptr<plugin::PluginContext> pluginContext,
    std::unique_ptr<vendor::VendorRuntime> vendorRuntime,
    ILogger& logger)
    : m_pluginManager(std::move(pluginManager))
    , m_pluginContext(std::move(pluginContext))
    , m_vendorRuntime(std::move(vendorRuntime))
    , m_logger(logger)
{
}

Result<void>
PluginService::installPlugin(std::unique_ptr<plugin::IPlugin> plugin) {
    if (!plugin) return ErrorCode::InvalidArgument;

    MBOOT_TRY(m_pluginManager->load(std::move(plugin)));

    return m_pluginManager->initializeAll();
}

Result<void>
PluginService::removePlugin(const std::string& name) {
    return m_pluginManager->unload(name);
}

std::vector<std::string>
PluginService::listPlugins() const {
    return m_pluginManager->listPlugins();
}

Result<void>
PluginService::registerVendor(std::unique_ptr<vendor::IVendor> vendor) {
    return vendor::VendorFactory::registerVendor(std::move(vendor));
}

Result<void>
PluginService::initialize() {
    return m_vendorRuntime->initialize(vendor::VendorContext{});
}

Result<void>
PluginService::shutdown() noexcept {
    auto shutResult = m_pluginManager->shutdownAll();
    (void)m_vendorRuntime->shutdown();
    vendor::VendorFactory::clearRegistry();
    return shutResult;
}

// ============================================================
// Accessors (internal bridge for Runtime)
// ============================================================

plugin::PluginManager& PluginService::pluginManager() noexcept {
    return *m_pluginManager;
}

const plugin::PluginManager& PluginService::pluginManager() const noexcept {
    return *m_pluginManager;
}

plugin::PluginContext& PluginService::pluginContext() noexcept {
    return *m_pluginContext;
}

vendor::VendorRuntime& PluginService::vendorRuntime() noexcept {
    return *m_vendorRuntime;
}

const vendor::VendorRuntime& PluginService::vendorRuntime() const noexcept {
    return *m_vendorRuntime;
}

} // namespace runtime
} // namespace mbootcore
