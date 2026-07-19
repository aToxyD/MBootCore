#pragma once

#include "SessionId.hpp"
#include "ProtocolVersion.hpp"

#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/domain/ITransport.hpp>

namespace mbootcore::protocol {

struct ProtocolContext {
    ProtocolContext(ITransport& t, ILogger* l, SessionId sid) noexcept
        : transport(t)
        , logger(l)
        , sessionId(sid)
    {}

    ITransport&      transport;
    ILogger*         logger;
    const SessionId  sessionId;
    ProtocolVersion  version = ProtocolVersion{0, 0};
};

} // namespace mbootcore::protocol
