#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace mbootcore {

enum class StorageType : uint8_t {
    Unknown = 0,
    eMMC,
    UFS,
    NAND,
    NOR,
    SPI
};

inline std::string_view toString(StorageType type) noexcept {
    switch (type) {
        case StorageType::eMMC:    return "eMMC";
        case StorageType::UFS:     return "UFS";
        case StorageType::NAND:    return "NAND";
        case StorageType::NOR:     return "NOR";
        case StorageType::SPI:     return "SPI";
        default:                   return "Unknown";
    }
}

inline StorageType storageTypeFromName(std::string_view name) noexcept {
    if (name == "emmc" || name == "eMMC") return StorageType::eMMC;
    if (name == "ufs"  || name == "UFS")  return StorageType::UFS;
    if (name == "nand" || name == "NAND") return StorageType::NAND;
    if (name == "nor"  || name == "NOR")  return StorageType::NOR;
    if (name == "spi"  || name == "SPI")  return StorageType::SPI;
    return StorageType::Unknown;
}

struct StorageInfo {
    StorageType type{StorageType::Unknown};
    std::string name;
    uint64_t numSectors{0};
    uint32_t sectorSize{0};
    uint32_t pageSize{0};
    uint64_t capacity{0};

    uint64_t capacityBytes() const noexcept {
        if (capacity > 0) return capacity;
        if (sectorSize != 0 &&
            numSectors > std::numeric_limits<uint64_t>::max() / sectorSize) {
            return 0;
        }
        return numSectors * sectorSize;
    }
};

} // namespace mbootcore
