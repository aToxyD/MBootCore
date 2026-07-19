#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace mbootcore {

using ByteBuffer = std::vector<uint8_t>;

/// Protocol-independent version information.
struct ProtocolVersion {
    uint32_t major{0};
    uint32_t minor{0};

    bool operator==(const ProtocolVersion& o) const noexcept {
        return major == o.major && minor == o.minor;
    }
    bool operator!=(const ProtocolVersion& o) const noexcept {
        return !(*this == o);
    }
    bool operator<(const ProtocolVersion& o) const noexcept {
        return major < o.major || (major == o.major && minor < o.minor);
    }
};

/// Unique device identifiers — populated by Sahara V2 EXECUTE_REQ or V3 READ_CHIPID_V3.
struct DeviceId {
    uint32_t msmId{0};    ///< MSM chip ID (from EXECUTE_REQ client_cmd=2 or V3)
    uint32_t oemId{0};    ///< OEM ID (from EXECUTE_REQ client_cmd=2 or V3)
    uint32_t modelId{0};  ///< Model ID (from EXECUTE_REQ client_cmd=2 or V3)
    std::array<uint8_t, 32> pkhash{};  ///< OEM PK Hash, 32 raw bytes (EXECUTE_REQ client_cmd=3)
};

/// Sahara transfer mode — from HELLO_REQ mode field.
enum class SaharaMode : uint32_t {
    ImageTxPending = 0,  ///< Device has images to transfer
    CommandMode    = 1,  ///< Device is in command mode (memory debug, exec)
    StreamingMode  = 2,  ///< Device is in streaming mode
};

/// Device capability flags.
enum class DeviceCapability : uint32_t {
    None          = 0,
    V3Supported   = 1 << 0,  ///< Device supports Sahara V3 protocol
    MemoryDebug   = 1 << 1,  ///< Device supports MEMORY_DEBUG command
    StreamMode    = 1 << 2,  ///< Device supports streaming mode
    ResetSM       = 1 << 3,  ///< Device supports RESET_STATE_MACHINE_ID (0x13)
};

inline DeviceCapability operator|(DeviceCapability a, DeviceCapability b) noexcept {
    return static_cast<DeviceCapability>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline DeviceCapability operator&(DeviceCapability a, DeviceCapability b) noexcept {
    return static_cast<DeviceCapability>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool hasCapability(DeviceCapability caps, DeviceCapability flag) noexcept {
    return (static_cast<uint32_t>(caps) & static_cast<uint32_t>(flag)) != 0;
}

/// Full device information collected during Sahara handshake and discovery.
///
/// Fields are populated from:
///   HELLO_REQ        → version, supported, mode
///   READ_CHIPID_V3   → chipIdLo, chipIdHi, serialNumber, msmId, oemId, modelId (V3)
///   EXECUTE_REQ(1)   → serialNumber (V2 fallback)
///   EXECUTE_REQ(2)   → msmId, oemId, modelId (V2 fallback)
///   EXECUTE_REQ(3)   → pkhash (V2 fallback)
struct DeviceInfo {
    ProtocolVersion version{};        ///< Negotiated protocol version
    SaharaMode mode{SaharaMode::ImageTxPending};  ///< Transfer mode from HELLO_REQ

    uint32_t serialNumber{0};         ///< Device serial number
    uint32_t chipIdLo{0};             ///< Chip ID low 32 bits (V3 only)
    uint32_t chipIdHi{0};             ///< Chip ID high 32 bits (V3 only)

    DeviceId id{};                    ///< Device IDs (msmId, oemId, modelId, pkhash)

    DeviceCapability capabilities{DeviceCapability::None};  ///< Detected capabilities
};

/// Converts raw Sahara mode uint32 to the typed enum.
inline SaharaMode saharaModeFromRaw(uint32_t raw) noexcept {
    switch (raw) {
        case 0: return SaharaMode::ImageTxPending;
        case 1: return SaharaMode::CommandMode;
        case 2: return SaharaMode::StreamingMode;
        default: return SaharaMode::ImageTxPending;
    }
}

} // namespace mbootcore
