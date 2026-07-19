#pragma once

#include "IPacket.hpp"

namespace mbootcore {

class IPacketParser {
public:
    virtual ~IPacketParser() = default;

    virtual Result<std::unique_ptr<IPacket>> parse(const ByteBuffer& data) = 0;
    virtual bool isComplete(const ByteBuffer& data) const noexcept = 0;
    virtual size_t minPacketSize() const noexcept = 0;
};

} // namespace mbootcore
