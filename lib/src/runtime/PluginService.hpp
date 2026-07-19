#pragma once

#include <mbootcore/runtime/IPluginService.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/plugin/PluginManager.hpp>
#include <mbootcore/plugin/PluginContext.hpp>
#include <mbootcore/vendor/VendorRuntime.hpp>
#include <mbootcore/vendor/VendorFactory.hpp>

#include <memory>

namespace mbootcore {
namespace runtime {

class PluginService final : public IPluginService {
public:
    PluginService(
        std::unique_ptr<plugin::PluginManager> pluginManager,
        std::unique_ptr<plugin::PluginContext> pluginContext,
        std::unique_ptr<vendor::VendorRuntime> vendorRuntime,
        ILogger& logger);

    ~PluginService() override = default;

    // IPluginService
    Result<void> installPlugin(std::unique_ptr<plugin::IPlugin> plugin) override;
    Result<void> removePlugin(const std::string& name) override;
    std::vector<std::string> listPlugins() const override;
    Result<void> registerVendor(std::unique_ptr<vendor::IVendor> vendor) override;

    // Lifecycle (called by Runtime)
    Result<void> initialize();
    Result<void> shutdown() noexcept;

    // Accessors for Runtime's internal API bridge
    plugin::PluginManager& pluginManager() noexcept;
    const plugin::PluginManager& pluginManager() const noexcept;
    plugin::PluginContext& pluginContext() noexcept;
    vendor::VendorRuntime& vendorRuntime() noexcept;
    const vendor::VendorRuntime& vendorRuntime() const noexcept;

private:
    std::unique_ptr<plugin::PluginManager> m_pluginManager;
    std::unique_ptr<plugin::PluginContext> m_pluginContext;
    std::unique_ptr<vendor::VendorRuntime> m_vendorRuntime;
    [[maybe_unused]] ILogger& m_logger;
};

} // namespace runtime
} // namespace mbootcore
