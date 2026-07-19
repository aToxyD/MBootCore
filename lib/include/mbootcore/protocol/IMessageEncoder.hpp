#pragma once

#include "Packet.hpp"
#include "ProtocolResult.hpp"

#include "Event.hpp"
#include "Request.hpp"
#include "Response.hpp"

namespace mbootcore::protocol {

class IMessageEncoder {
public:
    virtual ~IMessageEncoder() = default;

    virtual ProtocolResult<Packet> encode(const Request&  request)  = 0;
    virtual ProtocolResult<Packet> encode(const Response& response) = 0;
    virtual ProtocolResult<Packet> encode(const Event&    event)    = 0;
};

} // namespace mbootcore::protocol
