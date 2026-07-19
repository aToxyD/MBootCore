#pragma once

#include "IPacket.hpp"

namespace mbootcore {

class IPacketSerializer {
public:
    virtual ~IPacketSerializer() = default;

    virtual Result<ByteBuffer> serialize(const IPacket& packet) = 0;
    virtual size_t headerSize() const noexcept = 0;
    virtual bool canSerialize(const IPacket& packet) const noexcept = 0;
};

} // namespace mbootcore
