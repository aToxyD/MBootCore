#pragma once

#include "ProtocolResult.hpp"

#include <mbootcore/domain/ITransport.hpp>

#include <memory>

namespace mbootcore::protocol {

class IProtocolDiscovery {
public:
    virtual ~IProtocolDiscovery() = default;

    virtual ProtocolResult<bool> probe(ITransport& transport) = 0;
};

} // namespace mbootcore::protocol
