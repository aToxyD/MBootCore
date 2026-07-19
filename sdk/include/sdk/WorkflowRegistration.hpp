#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace sdk {

struct WorkflowRegistration {
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    std::vector<std::string> requiredCapabilities;
    std::vector<std::string> optionalCapabilities;
    std::vector<std::string> compatibleVendors;
    std::vector<std::string> compatibleProtocols;
    std::vector<std::string> dependencies;
    bool isDefault{false};
    uint32_t priority{100};
};

} // namespace sdk
} // namespace mbootcore
