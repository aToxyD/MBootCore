#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

namespace mbootcore {

void SaharaPacketSerializer::writeU32(ByteBuffer& buf, uint32_t value) noexcept {
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void SaharaPacketSerializer::writeU64(ByteBuffer& buf, uint64_t value) noexcept {
    writeU32(buf, static_cast<uint32_t>(value & 0xFFFFFFFF));
    writeU32(buf, static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF));
}

bool SaharaPacketSerializer::canSerialize(const IPacket& packet) const noexcept {
    auto cmd = packet.command();
    return cmd >= 0x01 && cmd <= 0x13;
}

Result<ByteBuffer> SaharaPacketSerializer::serialize(const IPacket& packet) {
    if (!canSerialize(packet)) {
        return ErrorCode::InvalidPacket;
    }

    ByteBuffer buf;
    buf.reserve(packet.length());

    writeU32(buf, packet.command());
    writeU32(buf, packet.length());

    switch (packet.command()) {
    case 0x01: {
        auto& p = dynamic_cast<const HelloPacket&>(packet);
        writeU32(buf, p.version());
        writeU32(buf, p.versionSupported());
        writeU32(buf, p.cmdPacketLength());
        writeU32(buf, p.mode());
        buf.resize(packet.length());
        break;
    }
    case 0x02: {
        auto& p = dynamic_cast<const HelloResponsePacket&>(packet);
        writeU32(buf, p.version());
        writeU32(buf, p.versionSupported());
        writeU32(buf, p.status());
        writeU32(buf, p.mode());
        buf.resize(packet.length());
        break;
    }
    case 0x03: {
        auto& p = dynamic_cast<const ReadDataPacket&>(packet);
        writeU32(buf, p.imageId());
        writeU32(buf, p.dataOffset());
        writeU32(buf, p.dataLength());
        break;
    }
    case 0x04: {
        auto& p = dynamic_cast<const EndImageTransferPacket&>(packet);
        writeU32(buf, p.imageId());
        writeU32(buf, p.status());
        break;
    }
    case 0x05:
    case 0x07:
    case 0x08:
    case 0x0B:
        break;
    case 0x06: {
        auto& p = dynamic_cast<const DoneResponsePacket&>(packet);
        writeU32(buf, p.imageTxStatus());
        break;
    }
    case 0x09: {
        auto& p = dynamic_cast<const MemoryDebugPacket&>(packet);
        writeU32(buf, p.memTableAddr());
        writeU32(buf, p.memTableLen());
        break;
    }
    case 0x0A: {
        auto& p = dynamic_cast<const ReadChipIdPacket&>(packet);
        writeU32(buf, p.chipIdLo());
        writeU32(buf, p.chipIdHi());
        if (p.isV3()) {
            writeU32(buf, p.serialNum());
            writeU32(buf, p.msmId());
            writeU32(buf, p.oemId());
            writeU32(buf, p.modelId());
            buf.resize(packet.length());
        }
        break;
    }
    case 0x0C: {
        auto& p = dynamic_cast<const CommandSwitchModePacket&>(packet);
        writeU32(buf, p.mode());
        break;
    }
    case 0x0D: {
        auto& p = dynamic_cast<const CommandExecPacket&>(packet);
        writeU32(buf, p.clientCmd());
        break;
    }
    case 0x0E: {
        auto& p = dynamic_cast<const CommandExecResponsePacket&>(packet);
        writeU32(buf, p.clientCmd());
        writeU32(buf, p.respLength());
        break;
    }
    case 0x0F: {
        auto& p = dynamic_cast<const ExecuteDataPacket&>(packet);
        writeU32(buf, p.clientCmd());
        writeU32(buf, p.dataLength());
        break;
    }
    case 0x10: {
        auto& p = dynamic_cast<const ExecuteDataResponsePacket&>(packet);
        writeU32(buf, p.clientCmd());
        writeU32(buf, p.status());
        break;
    }
    case 0x11: {
        auto& p = dynamic_cast<const RxDataPacket&>(packet);
        writeU32(buf, p.imageId());
        writeU32(buf, p.dataOffset());
        writeU32(buf, p.dataLength());
        break;
    }
    case 0x12: {
        auto& p = dynamic_cast<const RxDataResponsePacket&>(packet);
        writeU32(buf, p.imageId());
        writeU32(buf, p.status());
        break;
    }
    case 0x13:
        break;
    default:
        return ErrorCode::InvalidPacket;
    }

    return buf;
}

} // namespace mbootcore
