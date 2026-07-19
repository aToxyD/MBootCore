#pragma once

#include <mbootcore/domain/TransportTypes.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mbootcore {
namespace discovery {

enum class Vendor : uint32_t {
    Unknown    = 0,
    Qualcomm   = 1,
    MediaTek   = 2,
    UNISOC     = 3,
    Samsung    = 4,
    Rockchip   = 5,
    Spreadtrum = 6,
    Apple      = 7,
    Google     = 8,
    Huawei     = 9,
    Custom     = 0xFF
};

// Note: Only Vendor::Qualcomm has production-ready protocol
// implementations (Sahara + Firehose). MediaTek and UNISOC have
// scaffold implementations gated by MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS.
// All other vendor entries are forward-looking enum reservations
// with no protocol implementation.

enum class BootMode : uint32_t {
    Unknown       = 0,
    BootROM       = 1,
    EDL           = 2,
    Firehose      = 3,
    Fastboot      = 4,
    ADB           = 5,
    Recovery      = 6,
    DownloadMode  = 7,
    Preloader     = 8,
    BROM          = 9,
    Normal        = 10,
    Download      = 11,
    Custom        = 0xFF
};

using TransportType = mbootcore::transport::TransportType;

enum class ProtocolType : uint32_t {
    Unknown         = 0,
    Sahara          = 1,
    Firehose        = 2,
    Fastboot        = 3,
    MediaTekBROM    = 4,
    MediaTekDA      = 5,
    UNISOCBootROM   = 6,
    UNISOCFDL       = 7,
    USBStream       = 8,
    Custom          = 0xFF
};

struct DeviceDescriptor {
    Vendor        vendor{Vendor::Unknown};
    BootMode      bootMode{BootMode::Unknown};
    TransportType transport{TransportType::Unknown};
    ProtocolType  protocolHint{ProtocolType::Unknown};

    uint16_t usbVid{0};
    uint16_t usbPid{0};

    std::string serialPort;
    int         serialBaudRate{115200};

    std::string tcpHost;
    uint16_t    tcpPort{0};

    std::string friendlyName;
    std::string connectionPath;

    std::unordered_map<std::string, std::string> capabilities;
    std::unordered_map<std::string, std::string> properties;

    bool isUsb() const noexcept { return transport == TransportType::USB; }
    bool isSerial() const noexcept { return transport == TransportType::Serial; }
    bool isTcp() const noexcept { return transport == TransportType::TCP; }
    bool isVirtual() const noexcept { return transport == TransportType::Virtual; }
    bool isValid() const noexcept { return vendor != Vendor::Unknown || !connectionPath.empty(); }
};

} // namespace discovery
} // namespace mbootcore
