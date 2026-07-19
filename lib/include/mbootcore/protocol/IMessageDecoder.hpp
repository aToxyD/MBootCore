#pragma once

#include "Packet.hpp"
#include "ProtocolResult.hpp"

#include "Event.hpp"
#include "Request.hpp"
#include "Response.hpp"

namespace mbootcore::protocol {

class IMessageDecoder {
public:
    virtual ~IMessageDecoder() = default;

    virtual ProtocolResult<Request>  decodeRequest(const Packet& packet)  = 0;
    virtual ProtocolResult<Response> decodeResponse(const Packet& packet) = 0;
    virtual ProtocolResult<Event>    decodeEvent(const Packet& packet)    = 0;
};

} // namespace mbootcore::protocol
