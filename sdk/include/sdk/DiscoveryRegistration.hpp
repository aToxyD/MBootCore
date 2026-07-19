#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct DiscoveryRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    discovery::Vendor vendor{discovery::Vendor::Custom};
    std::vector<discovery::TransportType> supportedTransports;
    std::vector<uint16_t> usbVids;
    std::vector<uint16_t> usbPids;
    std::vector<uint32_t> serialBaudRates;
    std::vector<std::string> discoveryMethods;
    std::vector<std::string> dependencies;
    uint32_t scanTimeoutMs{5000};
};

} // namespace sdk
} // namespace mbootcore
