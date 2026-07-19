#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct VendorRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    discovery::Vendor vendorId{discovery::Vendor::Custom};
    std::string sdkVersion;
    std::string author;
    std::string license;
    std::string homeUrl;
    std::string supportContact;
    std::vector<discovery::ProtocolType> supportedProtocols;
    std::vector<uint16_t> usbVids;
    std::vector<uint16_t> usbPids;
    std::vector<std::string> dependencies;
    bool requiresSignedFirmware{false};
    bool supportsMultipleFlash{true};
};

} // namespace sdk
} // namespace mbootcore
