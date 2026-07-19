/// Dummy Vendor Plugin Example
/// Minimal example registering a custom third-party vendor.

#include <sdk/VendorSDK.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/VendorSDKFactory.hpp>

#include <iostream>

namespace sdk = mbootcore::sdk;
namespace discovery = mbootcore::discovery;

int main() {
    auto vendorSdk = sdk::VendorSDKFactory::createDefault();

    sdk::VendorRegistration vendor;
    vendor.name = "MyCustomVendor";
    vendor.displayName = "My Custom Vendor";
    vendor.description = "A third-party hardware vendor";
    vendor.version = "1.0.0";
    vendor.vendorId = discovery::Vendor::Custom;
    vendor.author = "Third Party Developer";
    vendor.license = "Proprietary";
    vendor.usbVids.push_back(0x1234);
    vendor.requiresSignedFirmware = false;
    vendorSdk->registerVendor(vendor);

    auto report = vendorSdk->finalize();

    if (report.valid) {
        std::cout << "Dummy vendor SDK registration successful!" << std::endl;
        std::cout << "Registered " << report.registeredVendors.size() << " vendor(s)" << std::endl;
    } else {
        std::cerr << "SDK registration failed:" << std::endl;
        for (const auto& err : report.errors) {
            std::cerr << "  - " << err << std::endl;
        }
        return 1;
    }

    sdk::PluginManifest manifest;
    manifest.setPluginName("MyCustomVendorPlugin");
    manifest.setPluginVersion("1.0.0");
    manifest.setSDKVersion("2.0.0");
    manifest.setVendor("MyCustomVendor");
    manifest.setAuthor("Third Party Developer");
    manifest.setLicense("Proprietary");
    manifest.setDescription("A third-party hardware vendor plugin");

    std::cout << "\nGenerated Plugin Manifest:" << std::endl;
    std::cout << manifest.toJson() << std::endl;

    return 0;
}
