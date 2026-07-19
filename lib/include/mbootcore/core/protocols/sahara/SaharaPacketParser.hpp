#pragma once

#include "mbootcore/domain/IPacketParser.hpp"

namespace mbootcore {

class SaharaPacketParser : public IPacketParser {
public:
    Result<std::unique_ptr<IPacket>> parse(const ByteBuffer& data) override;
    bool isComplete(const ByteBuffer& data) const noexcept override;
    size_t minPacketSize() const noexcept override { return 8; }

private:
    static uint32_t readU32(const ByteBuffer& data, size_t offset) noexcept;
    /// Returns the protocol-defined minimum packet length for a Sahara command.
    ///
    /// This performs structural validation only. Semantic validation remains the
    /// responsibility of parseCommand() and higher protocol layers.
    static uint32_t requiredPacketLength(uint32_t command) noexcept;
    Result<std::unique_ptr<IPacket>> parseCommand(const ByteBuffer& data, uint32_t cmd);
};

} // namespace mbootcore
