#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct JobRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    std::string jobType;
    std::vector<std::string> compatibleVendors;
    std::vector<std::string> compatibleProtocols;
    std::vector<std::string> requiredCapabilities;
    std::vector<std::string> dependencies;
    bool supportsCancellation{true};
    bool supportsProgress{true};
    uint32_t defaultPriority{50};
};

} // namespace sdk
} // namespace mbootcore
