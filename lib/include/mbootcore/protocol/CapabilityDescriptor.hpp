#pragma once

#include "CapabilityId.hpp"

#include <string_view>

namespace mbootcore::protocol {

class CapabilityDescriptor {
public:
    CapabilityId id;
    std::string_view name;
    std::string_view description;
};

} // namespace mbootcore::protocol
