#pragma once

#include <mbootcore/domain/DeviceTypes.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mbootcore {

using BootMode = discovery::BootMode;

inline std::string_view toString(BootMode mode) noexcept {
    switch (mode) {
        case BootMode::Download:     return "Download";
        case BootMode::Normal:       return "Normal";
        case BootMode::Recovery:     return "Recovery";
        case BootMode::Fastboot:     return "Fastboot";
        case BootMode::Firehose:     return "Firehose";
        case BootMode::EDL:          return "EmergencyDownload";
        case BootMode::BootROM:      return "BootROM";
        case BootMode::ADB:          return "ADB";
        case BootMode::DownloadMode: return "DownloadMode";
        case BootMode::Preloader:    return "Preloader";
        case BootMode::BROM:         return "BROM";
        default:                     return "Unknown";
    }
}

struct GenericDeviceInfo {
    std::string vendor;
    std::string name;
    std::string chipset;
    std::string serialNumber;
    std::string protocolName;
    uint32_t protocolVersion{0};
    BootMode bootMode{BootMode::Unknown};
    std::string storageType;
    uint64_t storageCapacity{0};
};

} // namespace mbootcore
