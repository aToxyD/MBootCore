#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/vendor/VendorFactory.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <string>

namespace mbootcore {
namespace vendor {

class IVendor;
class IVendorPlugin;

class VendorRuntime {
public:
    VendorRuntime();
    ~VendorRuntime();

    VendorRuntime(const VendorRuntime&) = delete;
    VendorRuntime& operator=(const VendorRuntime&) = delete;
    VendorRuntime(VendorRuntime&&) = delete;
    VendorRuntime& operator=(VendorRuntime&&) = delete;

    Result<void> initialize(const VendorContext& context);
    Result<void> shutdown() noexcept;
    Result<void> loadVendor(const std::string& vendorId);
    Result<void> unloadVendor(const std::string& vendorId);
    Result<void> loadPlugin(std::unique_ptr<IVendorPlugin> plugin);
    Result<void> unloadPlugin(const std::string& pluginId);
    Result<void> createRuntimeContext(const std::string& vendorId);

    IVendor* activeVendor() const noexcept;
    VendorRegistry& registry() noexcept;
    const VendorRegistry& registry() const noexcept;
    const VendorContext& context() const noexcept;
    VendorContext& mutableContext() noexcept;
    bool isInitialized() const noexcept;

private:
    VendorRegistry m_registry;
    VendorContext m_context;
    std::string m_activeVendorId;
    bool m_initialized{false};
};

} // namespace vendor
} // namespace mbootcore
