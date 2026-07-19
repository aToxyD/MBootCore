#pragma once

#include <sdk/VendorSDK.hpp>
#include <memory>

namespace mbootcore {
namespace sdk {

class VendorSDKFactory {
public:
    VendorSDKFactory() = default;

    static std::unique_ptr<VendorSDK> createDefault();
    static std::unique_ptr<VendorSDK> createFromManifest(const PluginManifest& manifest);
    static std::unique_ptr<VendorSDK> createMinimal();

private:
    static void registerCoreComponents(VendorSDK& sdk);
};

} // namespace sdk
} // namespace mbootcore
