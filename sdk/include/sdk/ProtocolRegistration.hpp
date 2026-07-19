#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct ProtocolRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    discovery::ProtocolType protocolType{discovery::ProtocolType::Custom};
    discovery::Vendor vendor{discovery::Vendor::Custom};
    std::vector<discovery::BootMode> supportedBootModes;
    std::vector<discovery::TransportType> supportedTransports;
    uint32_t maxPacketSize{1024};
    uint32_t defaultTimeoutMs{5000};
    bool supportsChecksum{false};
    bool supportsMultiPacket{true};
    std::vector<std::string> dependencies;
};

} // namespace sdk
} // namespace mbootcore
