#include <sdk/VendorSDKFactory.hpp>

namespace mbootcore {
namespace sdk {

std::unique_ptr<VendorSDK> VendorSDKFactory::createDefault() {
    auto sdk = std::make_unique<VendorSDK>();
    registerCoreComponents(*sdk);
    return sdk;
}

std::unique_ptr<VendorSDK> VendorSDKFactory::createFromManifest(const PluginManifest& manifest) {
    (void)manifest;
    auto sdk = std::make_unique<VendorSDK>();
    registerCoreComponents(*sdk);
    return sdk;
}

std::unique_ptr<VendorSDK> VendorSDKFactory::createMinimal() {
    return std::make_unique<VendorSDK>();
}

void VendorSDKFactory::registerCoreComponents(VendorSDK& sdk) {
    (void)sdk;
}

} // namespace sdk
} // namespace mbootcore
