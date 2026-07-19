#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mbootcore {

enum class FlashCapability : uint64_t {
    None               = 0,
    Read               = 1ULL << 0,
    Write              = 1ULL << 1,
    Erase              = 1ULL << 2,
    Reset              = 1ULL << 3,
    UploadLoader       = 1ULL << 4,
    StorageInfo        = 1ULL << 5,
    Partitions         = 1ULL << 6,
    Peek               = 1ULL << 7,
    Poke               = 1ULL << 8,
    Patch              = 1ULL << 9,
    PowerReset         = 1ULL << 10,
    XmlCommands        = 1ULL << 11,
    Streaming          = 1ULL << 12,
    MemoryDebug        = 1ULL << 13,
    SparseImages       = 1ULL << 14,
    Sha256Digest       = 1ULL << 15,
    ConfigureMemory    = 1ULL << 16,
    BootFromStorage    = 1ULL << 17,
    All                = ~0ULL
};

inline FlashCapability operator|(FlashCapability a, FlashCapability b) noexcept {
    return static_cast<FlashCapability>(
        static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline FlashCapability operator&(FlashCapability a, FlashCapability b) noexcept {
    return static_cast<FlashCapability>(
        static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline bool hasCapability(FlashCapability caps, FlashCapability flag) noexcept {
    return (static_cast<uint64_t>(caps) & static_cast<uint64_t>(flag)) != 0;
}

inline std::string_view toString(FlashCapability cap) noexcept {
    switch (cap) {
        case FlashCapability::Read:            return "Read";
        case FlashCapability::Write:           return "Write";
        case FlashCapability::Erase:           return "Erase";
        case FlashCapability::Reset:           return "Reset";
        case FlashCapability::UploadLoader:    return "UploadLoader";
        case FlashCapability::StorageInfo:     return "StorageInfo";
        case FlashCapability::Partitions:      return "Partitions";
        case FlashCapability::Peek:            return "Peek";
        case FlashCapability::Poke:            return "Poke";
        case FlashCapability::Patch:           return "Patch";
        case FlashCapability::PowerReset:      return "PowerReset";
        case FlashCapability::XmlCommands:     return "XmlCommands";
        case FlashCapability::Streaming:       return "Streaming";
        case FlashCapability::MemoryDebug:     return "MemoryDebug";
        case FlashCapability::SparseImages:    return "SparseImages";
        case FlashCapability::Sha256Digest:    return "Sha256Digest";
        case FlashCapability::ConfigureMemory: return "ConfigureMemory";
        case FlashCapability::BootFromStorage: return "BootFromStorage";
        default:                               return "Unknown";
    }
}

} // namespace mbootcore
