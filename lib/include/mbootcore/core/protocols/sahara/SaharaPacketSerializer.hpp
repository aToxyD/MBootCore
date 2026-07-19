#pragma once

#include "mbootcore/domain/IPacketSerializer.hpp"

namespace mbootcore {

class SaharaPacketSerializer : public IPacketSerializer {
public:
    Result<ByteBuffer> serialize(const IPacket& packet) override;
    size_t headerSize() const noexcept override { return 8; }
    bool canSerialize(const IPacket& packet) const noexcept override;

private:
    static void writeU32(ByteBuffer& buf, uint32_t value) noexcept;
    static void writeU64(ByteBuffer& buf, uint64_t value) noexcept;
};

} // namespace mbootcore
