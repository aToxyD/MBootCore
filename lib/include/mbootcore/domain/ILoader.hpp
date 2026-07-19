#pragma once

#include "Types.hpp"
#include "Error.hpp"

#include <string>

namespace mbootcore {

struct LoaderInfo {
    std::string filename;
    std::string targetDevice;
    ProtocolVersion version;
    uint32_t imageType{0};
    DeviceId compatibleId;
};

class ILoader {
public:
    virtual ~ILoader() = default;

    virtual const LoaderInfo& info() const noexcept = 0;
    virtual const ByteBuffer& data() const noexcept = 0;
    virtual Result<bool> isCompatibleWith(const DeviceId& deviceId) const = 0;
    virtual Result<void> parse() = 0;
};

} // namespace mbootcore
