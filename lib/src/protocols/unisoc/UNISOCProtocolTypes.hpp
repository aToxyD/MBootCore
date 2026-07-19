#pragma once

#ifdef MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS

#include <cstdint>
#include <cstring>
#include <vector>

// ======================================================================
// UNISOC Scaffold Protocol — Wire Format Types
//
// This is a REFERENCE SCAFFOLD, not a production implementation.
// It demonstrates the Protocol Platform extensibility pattern using a
// UNISOC-like wire format. It is NOT tested against real hardware and
// is NOT suitable for production use.
// ======================================================================

namespace mbootcore::protocol::unisoc {

// Wire format:
//   Request:  [Command:1] [PayloadLength:2 LE] [Payload:variable]
//   Response: [Status:1]  [PayloadLength:2 LE] [Payload:variable]

inline constexpr uint32_t UnisocProtocolId  = 0x554E4930; // "UNI0"
inline constexpr uint32_t UnisocMajorVersion = 1;
inline constexpr uint32_t UnisocMinorVersion = 0;

enum class UnisocCommand : uint8_t {
    Probe       = 0x01,
    Handshake   = 0x02,
    GetVersion  = 0x03,
    GetChipInfo = 0x04,
};

enum class UnisocStatus : uint8_t {
    Success   = 0x00,
    Unknown   = 0x01,
    Error     = 0x02,
    Busy      = 0x03,
    Unsupported = 0x04,
};

struct ProbeResponse {
    uint8_t bootMode;
};

struct HandshakeResponse {
    uint8_t acceptedMajor;
    uint8_t acceptedMinor;
};

struct VersionResponse {
    uint8_t major;
    uint8_t minor;
    char    chip[16];
};

struct ChipInfoResponse {
    uint32_t chipId;
    uint32_t hardwareRev;
    uint32_t firmwareVer;
};

inline std::vector<uint8_t> makeRequestPacket(UnisocCommand cmd,
                                               const uint8_t* payload,
                                               uint16_t payloadLen)
{
    std::vector<uint8_t> packet;
    packet.reserve(3 + payloadLen);
    packet.push_back(static_cast<uint8_t>(cmd));
    packet.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    packet.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
    if (payload && payloadLen > 0)
        packet.insert(packet.end(), payload, payload + payloadLen);
    return packet;
}

inline std::vector<uint8_t> makeResponsePacket(UnisocStatus status,
                                                const uint8_t* payload,
                                                uint16_t payloadLen)
{
    std::vector<uint8_t> packet;
    packet.reserve(3 + payloadLen);
    packet.push_back(static_cast<uint8_t>(status));
    packet.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    packet.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
    if (payload && payloadLen > 0)
        packet.insert(packet.end(), payload, payload + payloadLen);
    return packet;
}

} // namespace mbootcore::protocol::unisoc

#else
#error "MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS is not defined. \
Vendor scaffold protocols are not available in this build. \
Enable the CMake option to use them."
#endif
