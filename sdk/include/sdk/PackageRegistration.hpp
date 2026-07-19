#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct PackageRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    std::vector<std::string> supportedFileExtensions;
    std::vector<std::string> requiredTools;
    std::vector<discovery::Vendor> compatibleVendors;
    std::vector<discovery::ProtocolType> compatibleProtocols;
    std::vector<std::string> dependencies;
    bool requiresSignature{false};
    bool supportsCompression{false};
    uint64_t maxPackageSize{0};
};

} // namespace sdk
} // namespace mbootcore
