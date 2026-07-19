#pragma once

#include <memory>

namespace mbootcore::protocol {

class IProtocolDiscovery;
class IProtocolSession;

class IProtocolFactory {
public:
    virtual ~IProtocolFactory() = default;

    virtual std::unique_ptr<IProtocolDiscovery> createDiscovery() = 0;
    virtual std::unique_ptr<IProtocolSession>   createSession()   = 0;
};

} // namespace mbootcore::protocol
