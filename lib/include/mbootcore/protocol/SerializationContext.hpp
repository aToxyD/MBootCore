#pragma once

#include "CapabilitySet.hpp"
#include "ProtocolVersion.hpp"

namespace mbootcore::protocol {

enum class Endianness : uint8_t {
    Little,
    Big
};

struct SerializationContext {
    ProtocolVersion version;
    Endianness      endianness    = Endianness::Little;
    CapabilitySet   negotiatedCapabilities;
};

} // namespace mbootcore::protocol
