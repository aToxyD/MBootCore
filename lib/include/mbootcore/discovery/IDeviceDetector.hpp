#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/domain/Error.hpp>

#include <vector>
#include <string>

namespace mbootcore {
namespace discovery {

class IDeviceDetector {
public:
    virtual ~IDeviceDetector() = default;

    virtual std::string name() const = 0;

    virtual Result<std::vector<DeviceDescriptor>> enumerate() = 0;

    virtual Result<DeviceDescriptor> identify(const DeviceDescriptor& hint) = 0;

    virtual Result<void> probe(DeviceDescriptor& descriptor) = 0;

    virtual Result<void> refresh() = 0;
};

} // namespace discovery
} // namespace mbootcore
