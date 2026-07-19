#pragma once

#include <sdk/PluginManifest.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/PackageRegistration.hpp>

#include <string>
#include <vector>

namespace mbootcore {
namespace sdk {

struct VersionRange {
    std::string minVersion;
    std::string maxVersion;
};

struct CompatibilityReport {
    bool compatible{true};
    std::vector<std::string> issues;
    std::vector<std::string> warnings;
};

struct VendorCompatibilityReport : CompatibilityReport {
    std::string vendorName;
    std::string vendorVersion;
    VersionRange supportedSDKRange;
};

struct ProtocolCompatibilityReport : CompatibilityReport {
    std::string protocolName;
    std::string protocolVersion;
    bool transportCompatible{true};
    bool vendorCompatible{true};
};

struct TransportCompatibilityReport : CompatibilityReport {
    std::string transportName;
    std::string transportVersion;
    bool protocolCompatible{true};
    bool hotplugSupported{false};
};

struct PackageCompatibilityReport : CompatibilityReport {
    std::string packageName;
    std::string packageVersion;
    bool vendorCompatible{true};
    bool protocolCompatible{true};
};

struct SDKVersionReport {
    std::string currentSDKVersion;
    std::string minSupported;
    std::string maxSupported;
    bool versionCompatible{true};
    std::vector<std::string> breakingChanges;
};

class PluginCompatibility {
public:
    PluginCompatibility();

    VendorCompatibilityReport checkVendorCompatibility(
        const VendorRegistration& reg, const PluginManifest& manifest) const;

    ProtocolCompatibilityReport checkProtocolCompatibility(
        const ProtocolRegistration& reg, const PluginManifest& manifest) const;

    TransportCompatibilityReport checkTransportCompatibility(
        const TransportRegistration& reg, const PluginManifest& manifest) const;

    PackageCompatibilityReport checkPackageCompatibility(
        const PackageRegistration& reg, const PluginManifest& manifest) const;

    SDKVersionReport checkSDKVersion(const PluginManifest& manifest) const;

    CompatibilityReport checkAll(const PluginManifest& manifest) const;

private:
    bool isVersionCompatible(const std::string& version,
                             const std::string& minVersion,
                             const std::string& maxVersion) const;

    static constexpr const char* kCurrentSDKVersion = "1.0.0";
    static constexpr const char* kMinSDKVersion = "1.0.0";
    static constexpr const char* kMaxSDKVersion = "1.0.0";
};

} // namespace sdk
} // namespace mbootcore
