#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/domain/Error.hpp>

#include <string>

namespace mbootcore {
namespace discovery {

class IProtocolNegotiator {
public:
    virtual ~IProtocolNegotiator() = default;

    virtual std::string name() const = 0;

    virtual NegotiationResult negotiate(const DeviceDescriptor& descriptor) = 0;
};

} // namespace discovery
} // namespace mbootcore
