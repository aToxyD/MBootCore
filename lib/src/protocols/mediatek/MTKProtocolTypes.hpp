#pragma once

#ifdef MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS

#include <cstdint>
#include <vector>

// ======================================================================
// MediaTek Scaffold Protocol — Wire Format Types
//
// This is a REFERENCE SCAFFOLD, not a production implementation.
// It demonstrates the Protocol Platform extensibility pattern using a
// MediaTek-like wire format. It is NOT tested against real hardware and
// is NOT suitable for production use.
// ======================================================================

namespace mbootcore::protocol::mediatek {

// Wire format (distinct from UNISOC to prove platform extensibility):
//   Request:  [Magic:2 = 0xAA55] [Command:2 LE] [PayloadLength:2 LE] [Payload:variable]
//   Response: [Magic:2 = 0x55AA] [Status:2 LE]  [PayloadLength:2 LE] [Payload:variable]

inline constexpr uint32_t MediatekProtocolId  = 0x4D544B00; // "MTK0"
inline constexpr uint32_t MediatekMajorVersion = 2;
inline constexpr uint32_t MediatekMinorVersion = 0;

enum class MTKCommand : uint16_t {
    Probe       = 0x1001,
    Handshake   = 0x1002,
    GetVersion  = 0x1003,
    GetHwCode   = 0x1004,
};

enum class MTKStatus : uint16_t {
    Success     = 0x0000,
    Unknown     = 0x0001,
    Fail        = 0x0002,
    Invalid     = 0x0003,
};

struct MTKProbeResponse {
    uint16_t bootRomVersion;
};

struct MTKHandshakeResponse {
    uint16_t acceptedMajor;
    uint16_t acceptedMinor;
};

struct MTKVersionResponse {
    uint16_t major;
    uint16_t minor;
    char     chip[32];
};

inline std::vector<uint8_t> makeRequestPacket(MTKCommand cmd,
                                               const uint8_t* payload,
                                               uint16_t payloadLen)
{
    std::vector<uint8_t> packet;
    packet.reserve(6 + payloadLen);
    packet.push_back(0xAA);
    packet.push_back(0x55);
    auto c = static_cast<uint16_t>(cmd);
    packet.push_back(static_cast<uint8_t>(c & 0xFF));
    packet.push_back(static_cast<uint8_t>((c >> 8) & 0xFF));
    packet.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    packet.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
    if (payload && payloadLen > 0)
        packet.insert(packet.end(), payload, payload + payloadLen);
    return packet;
}

inline std::vector<uint8_t> makeResponsePacket(MTKStatus status,
                                                const uint8_t* payload,
                                                uint16_t payloadLen)
{
    std::vector<uint8_t> packet;
    packet.reserve(6 + payloadLen);
    packet.push_back(0x55);
    packet.push_back(0xAA);
    auto s = static_cast<uint16_t>(status);
    packet.push_back(static_cast<uint8_t>(s & 0xFF));
    packet.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
    packet.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    packet.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
    if (payload && payloadLen > 0)
        packet.insert(packet.end(), payload, payload + payloadLen);
    return packet;
}

} // namespace mbootcore::protocol::mediatek

#else
#error "MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS is not defined. \
Vendor scaffold protocols are not available in this build. \
Enable the CMake option to use them."
#endif
