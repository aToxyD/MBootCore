#pragma once

#include <mbootcore/domain/Error.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <chrono>

namespace mbootcore {
namespace vendor {

enum class VendorFamily : uint32_t {
    Unknown = 0,
    Qualcomm,
    MediaTek,
    UNISOC,
    Rockchip,
    Samsung,
    Amlogic,
    NXP,
    Custom = 0xFF
};

// Note: MediaTek and UNISOC have scaffold protocol implementations
// (see MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS). Only Qualcomm has a
// production-ready protocol implementation.

enum class VendorMaturity : uint32_t {
    Experimental = 0,
    Scaffold     = 1,
    Preview      = 2,
    Production   = 3
};

enum class VendorCapability : uint32_t {
    None = 0,
    BootROM = 1 << 0,
    DownloadAgent = 1 << 1,
    LoaderUpload = 1 << 2,
    MemoryRead = 1 << 3,
    MemoryWrite = 1 << 4,
    Flash = 1 << 5,
    GPT = 1 << 6,
    Partition = 1 << 7,
    SecureBoot = 1 << 8,
    Authentication = 1 << 9,
    Reset = 1 << 10,
    Reboot = 1 << 11,
    Verify = 1 << 12,
    Streaming = 1 << 13,
    Logging = 1 << 14
};

inline VendorCapability operator|(VendorCapability a, VendorCapability b) {
    return static_cast<VendorCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline VendorCapability operator&(VendorCapability a, VendorCapability b) {
    return static_cast<VendorCapability>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool hasCapability(VendorCapability caps, VendorCapability test) {
    return (static_cast<uint32_t>(caps) & static_cast<uint32_t>(test)) != 0;
}

struct VendorInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    VendorFamily family{VendorFamily::Unknown};
    VendorMaturity maturity{VendorMaturity::Experimental};
    std::vector<std::string> supportedProtocols;
    VendorCapability capabilities{VendorCapability::None};
};

struct VendorStatistics {
    uint64_t devicesDetected{0};
    uint64_t successfulSessions{0};
    uint64_t failedSessions{0};
    uint64_t uploads{0};
    uint64_t flashes{0};
    std::chrono::milliseconds averageBootTime{0};
};

} // namespace vendor
} // namespace mbootcore
