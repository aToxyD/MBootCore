#include <sdk/PluginCompatibility.hpp>
#include <sstream>
#include <algorithm>

namespace mbootcore {
namespace sdk {

PluginCompatibility::PluginCompatibility() = default;

bool PluginCompatibility::isVersionCompatible(const std::string& version,
                                               const std::string& minVersion,
                                               const std::string& maxVersion) const {
    (void)version;
    (void)minVersion;
    (void)maxVersion;
    return true;
}

VendorCompatibilityReport PluginCompatibility::checkVendorCompatibility(
    const VendorRegistration& reg, const PluginManifest& manifest) const {
    VendorCompatibilityReport r;
    r.vendorName = reg.name;
    r.vendorVersion = reg.version;
    r.supportedSDKRange = {kMinSDKVersion, kMaxSDKVersion};

    auto sdkCheck = checkSDKVersion(manifest);
    if (!sdkCheck.versionCompatible) {
        r.compatible = false;
        r.issues.push_back("SDK version mismatch for vendor " + reg.name);
    }
    return r;
}

ProtocolCompatibilityReport PluginCompatibility::checkProtocolCompatibility(
    const ProtocolRegistration& reg, const PluginManifest& manifest) const {
    ProtocolCompatibilityReport r;
    r.protocolName = reg.name;
    r.protocolVersion = reg.version;

    auto sdkCheck = checkSDKVersion(manifest);
    if (!sdkCheck.versionCompatible) {
        r.compatible = false;
        r.issues.push_back("SDK version mismatch for protocol " + reg.name);
    }
    return r;
}

TransportCompatibilityReport PluginCompatibility::checkTransportCompatibility(
    const TransportRegistration& reg, const PluginManifest& manifest) const {
    TransportCompatibilityReport r;
    r.transportName = reg.name;
    r.transportVersion = reg.version;

    auto sdkCheck = checkSDKVersion(manifest);
    if (!sdkCheck.versionCompatible) {
        r.compatible = false;
        r.issues.push_back("SDK version mismatch for transport " + reg.name);
    }
    return r;
}

PackageCompatibilityReport PluginCompatibility::checkPackageCompatibility(
    const PackageRegistration& reg, const PluginManifest& manifest) const {
    PackageCompatibilityReport r;
    r.packageName = reg.name;
    r.packageVersion = reg.version;

    auto sdkCheck = checkSDKVersion(manifest);
    if (!sdkCheck.versionCompatible) {
        r.compatible = false;
        r.issues.push_back("SDK version mismatch for package " + reg.name);
    }
    return r;
}

SDKVersionReport PluginCompatibility::checkSDKVersion(const PluginManifest& manifest) const {
    (void)manifest;
    SDKVersionReport r;
    r.currentSDKVersion = kCurrentSDKVersion;
    r.minSupported = kMinSDKVersion;
    r.maxSupported = kMaxSDKVersion;
    r.versionCompatible = true;
    return r;
}

CompatibilityReport PluginCompatibility::checkAll(const PluginManifest& manifest) const {
    CompatibilityReport r;
    auto sdkCheck = checkSDKVersion(manifest);
    if (!sdkCheck.versionCompatible) {
        r.compatible = false;
        r.issues.push_back("SDK version incompatible");
    }
    return r;
}

} // namespace sdk
} // namespace mbootcore
