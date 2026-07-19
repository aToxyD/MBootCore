#pragma once

#include "Types.hpp"
#include "Error.hpp"

#include <memory>
#include <functional>

namespace mbootcore {

class IPacket {
public:
    virtual ~IPacket() = default;

    virtual uint32_t command() const noexcept = 0;
    virtual uint32_t length() const noexcept = 0;
    virtual std::string toString() const = 0;
    virtual std::unique_ptr<IPacket> clone() const = 0;
};

using PacketHandler = std::function<Result<void>(std::unique_ptr<IPacket>)>;

} // namespace mbootcore
