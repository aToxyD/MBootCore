#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

namespace mbootcore {

uint32_t SaharaPacketParser::readU32(const ByteBuffer& data, size_t offset) noexcept {
    if (offset + 4 > data.size()) return 0;
    return static_cast<uint32_t>(data[offset])
         | (static_cast<uint32_t>(data[offset + 1]) << 8)
         | (static_cast<uint32_t>(data[offset + 2]) << 16)
         | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

uint32_t SaharaPacketParser::requiredPacketLength(uint32_t command) noexcept {
    switch (command) {
    case 0x01: return 48; // HelloPacket::length()
    case 0x02: return 48; // HelloResponsePacket::length()
    case 0x03: return 20; // ReadDataPacket::length()
    case 0x04: return 16; // EndImageTransferPacket::length()
    case 0x05: return  8; // DonePacket::length()
    case 0x06: return 12; // DoneResponsePacket::length()
    case 0x07: return  8; // ResetPacket::length()
    case 0x08: return  8; // ResetResponsePacket::length()
    case 0x09: return 16; // MemoryDebugPacket::length()
    case 0x0A: return 16; // ReadChipIdPacket::length() — V2 min; V3 (40) detected in parseCommand
    case 0x0B: return  8; // CommandReadyPacket::length()
    case 0x0C: return 12; // CommandSwitchModePacket::length()
    case 0x0D: return 12; // CommandExecPacket::length()
    case 0x0E: return 16; // CommandExecResponsePacket::length()
    case 0x0F: return 16; // ExecuteDataPacket::length()
    case 0x10: return 16; // ExecuteDataResponsePacket::length()
    case 0x11: return 20; // RxDataPacket::length()
    case 0x12: return 16; // RxDataResponsePacket::length()
    case 0x13: return  8; // ResetStateMachinePacket::length()
    default:   return  0; // Unknown commands are validated by parseCommand()
    }
}

bool SaharaPacketParser::isComplete(const ByteBuffer& data) const noexcept {
    if (data.size() < 8) return false;
    auto len = readU32(data, 4);
    return data.size() >= len;
}

Result<std::unique_ptr<IPacket>> SaharaPacketParser::parse(const ByteBuffer& data) {
    if (data.size() < 8) {
        return ErrorCode::InvalidPacket;
    }

    auto cmd = readU32(data, 0);
    auto len = readU32(data, 4);

    if (data.size() < len) {
        return ErrorCode::InvalidPacket;
    }

    const uint32_t minimumLength = requiredPacketLength(cmd);

    if (len < minimumLength) {
        return ErrorCode::InvalidPacket;
    }

    return parseCommand(data, cmd);
}

Result<std::unique_ptr<IPacket>> SaharaPacketParser::parseCommand(const ByteBuffer& data, uint32_t cmd) {
    switch (cmd) {
    case 0x01: {
        auto pkt = std::make_unique<HelloPacket>(
            readU32(data, 8),
            readU32(data, 12),
            readU32(data, 16),
            readU32(data, 20)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x02: {
        auto pkt = std::make_unique<HelloResponsePacket>(
            readU32(data, 8),
            readU32(data, 12),
            readU32(data, 16),
            readU32(data, 20)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x03: {
        auto pkt = std::make_unique<ReadDataPacket>(
            readU32(data, 8),
            readU32(data, 12),
            readU32(data, 16)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x04: {
        auto pkt = std::make_unique<EndImageTransferPacket>(
            readU32(data, 8),
            readU32(data, 12)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x05:
        return Result<std::unique_ptr<IPacket>>(std::make_unique<DonePacket>());
    case 0x06: {
        auto pkt = std::make_unique<DoneResponsePacket>(
            readU32(data, 8)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x07:
        return Result<std::unique_ptr<IPacket>>(std::make_unique<ResetPacket>());
    case 0x08:
        return Result<std::unique_ptr<IPacket>>(std::make_unique<ResetResponsePacket>());
    case 0x09: {
        auto pkt = std::make_unique<MemoryDebugPacket>(
            readU32(data, 8),
            readU32(data, 12)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x0A: {
        auto len = readU32(data, 4);
        auto isV3 = (len >= 40);
        auto pkt = std::make_unique<ReadChipIdPacket>(
            readU32(data, 8),
            readU32(data, 12),
            isV3 ? readU32(data, 16) : 0,
            isV3 ? readU32(data, 20) : 0,
            isV3 ? readU32(data, 24) : 0,
            isV3 ? readU32(data, 28) : 0
        );
        pkt->setV3(isV3);
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x0B:
        return Result<std::unique_ptr<IPacket>>(std::make_unique<CommandReadyPacket>());
    case 0x0C: {
        auto pkt = std::make_unique<CommandSwitchModePacket>(
            readU32(data, 8)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x0D: {
        auto pkt = std::make_unique<CommandExecPacket>(
            readU32(data, 8)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x0E: {
        auto pkt = std::make_unique<CommandExecResponsePacket>(
            readU32(data, 8),
            readU32(data, 12)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x0F: {
        auto pkt = std::make_unique<ExecuteDataPacket>(
            readU32(data, 8),
            readU32(data, 12)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x10: {
        auto pkt = std::make_unique<ExecuteDataResponsePacket>(
            readU32(data, 8),
            readU32(data, 12)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x11: {
        auto pkt = std::make_unique<RxDataPacket>(
            readU32(data, 8),
            readU32(data, 12),
            readU32(data, 16)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x12: {
        auto pkt = std::make_unique<RxDataResponsePacket>(
            readU32(data, 8),
            readU32(data, 12)
        );
        return Result<std::unique_ptr<IPacket>>(std::move(pkt));
    }
    case 0x13:
        return Result<std::unique_ptr<IPacket>>(std::make_unique<ResetStateMachinePacket>());
    default:
        return ErrorCode::InvalidPacket;
    }
}

} // namespace mbootcore
