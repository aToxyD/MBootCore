#pragma once

#include "ProtocolId.hpp"
#include "ProtocolVersion.hpp"

#include <memory>

namespace mbootcore::protocol {

class IProtocolFactory;

class IProtocol {
public:
    virtual ~IProtocol() = default;

    virtual ProtocolId      id()      const = 0;
    virtual ProtocolVersion version() const = 0;

    virtual std::unique_ptr<IProtocolFactory> factory() const = 0;
};

} // namespace mbootcore::protocol
