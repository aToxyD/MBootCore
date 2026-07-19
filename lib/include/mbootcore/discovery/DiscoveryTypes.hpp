#pragma once

#include <mbootcore/domain/DeviceTypes.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace mbootcore {
namespace discovery {

struct DiscoveryResult {
    std::vector<DeviceDescriptor> devices;
    uint32_t elapsedMs{0};
    bool     timedOut{false};
    bool     error{false};
    std::string errorMessage;
};

struct NegotiationResult {
    ProtocolType protocol{ProtocolType::Unknown};
    int          confidence{0};
    std::string  reason;
    DeviceDescriptor descriptor;
};

} // namespace discovery
} // namespace mbootcore
