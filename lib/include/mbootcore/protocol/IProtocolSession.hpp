#pragma once

#include "CapabilitySet.hpp"
#include "ProtocolResult.hpp"
#include "SessionId.hpp"

#include <mbootcore/domain/ITransport.hpp>

#include <memory>

namespace mbootcore::protocol {

class IProtocolSession {
public:
    virtual ~IProtocolSession() = default;

    virtual SessionId      id()           const = 0;
    virtual CapabilitySet  capabilities() const = 0;
    virtual bool           isOpen()       const = 0;

    virtual ProtocolResult<void> open(ITransport& transport)  = 0;
    virtual ProtocolResult<void> close()                      = 0;
};

} // namespace mbootcore::protocol
