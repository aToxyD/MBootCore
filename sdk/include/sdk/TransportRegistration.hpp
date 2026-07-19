#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct TransportRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    discovery::TransportType transportType{discovery::TransportType::Virtual};
    std::vector<discovery::ProtocolType> compatibleProtocols;
    uint32_t defaultBaudRate{115200};
    uint32_t maxTransferSize{65536};
    bool supportsHotplug{false};
    bool supportsReconnect{false};
    bool supportsKeepalive{false};
    std::vector<std::string> dependencies;
};

} // namespace sdk
} // namespace mbootcore
