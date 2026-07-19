#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>
#include <string>

namespace mbootcore {
namespace vendor {

class IVendorPlugin;

class VendorFactory {
public:
    static std::unique_ptr<IVendor> createVendor(VendorFamily family);
    static std::unique_ptr<IVendor> createVendor(const std::string& vendorId);
    static std::unique_ptr<IVendorPlugin> createPlugin(const std::string& pluginId);
    static std::unique_ptr<IVendor> createFromDescriptor(const discovery::DeviceDescriptor& descriptor);
    static std::unique_ptr<IVendor> createFromDiscovery(const discovery::DiscoveryResult& discovery);
    static std::unique_ptr<IVendor> createFromSession(const session::SessionConfig& config);
    static Result<void> registerVendor(std::unique_ptr<IVendor> vendor);
    static void clearRegistry();

private:
    static VendorRegistry& registry();
};

} // namespace vendor
} // namespace mbootcore
