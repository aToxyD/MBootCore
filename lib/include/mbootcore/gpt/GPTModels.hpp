#pragma once

#include "mbootcore/gpt/Guid.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace mbootcore {
namespace gpt {

// ── GPT Header (on-disk layout, 92 bytes, padded to 512) ──

struct GPTHeader {
    static constexpr uint64_t Signature = 0x5452415020494645ULL;
    static constexpr uint32_t Revision = 0x00010000;
    static constexpr uint32_t MinHeaderSize = 92;
    static constexpr uint32_t MinEntrySize = 128;
    static constexpr uint32_t DefaultEntryCount = 128;

    uint64_t signature{0};
    uint32_t revision{0};
    uint32_t headerSize{0};
    uint32_t headerCrc32{0};
    uint32_t reserved{0};
    uint64_t myLBA{0};
    uint64_t alternateLBA{0};
    uint64_t firstUsableLBA{0};
    uint64_t lastUsableLBA{0};
    Guid diskGUID;
    uint64_t partitionEntryLBA{0};
    uint32_t numberOfPartitionEntries{0};
    uint32_t sizeOfPartitionEntry{0};
    uint32_t partitionEntriesCRC32{0};

    bool isValid() const noexcept;
    bool hasValidSignature() const noexcept;
    uint64_t totalEntrySize() const noexcept;
    uint64_t lastEntryLBA() const noexcept;
};

// ── Partition Entry (on-disk, 128 bytes) ──

struct PartitionEntry {
    Guid partitionTypeGUID;
    Guid uniquePartitionGUID;
    uint64_t firstLBA{0};
    uint64_t lastLBA{0};
    uint64_t attributes{0};
    std::array<char16_t, 36> name{};

    bool isUsed() const noexcept;
    bool isValid() const noexcept;
    uint64_t sizeInLBAs() const noexcept;
    uint64_t sizeInBytes() const noexcept;
    std::string nameToString() const;

    static std::u16string utf8ToUtf16(std::string_view utf8);
    static std::string utf16ToUtf8(const char16_t* utf16, size_t len);
};

// ── Partition Type GUIDs ──

namespace PartitionTypes {
    inline constexpr Guid Unused{};
    inline constexpr Guid EFISystem{0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}};
    inline constexpr Guid MBRScheme{0x024DEE41, 0x33E7, 0x11D3, {0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F}};
    inline constexpr Guid BasicData{0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}};
    inline constexpr Guid LinuxFS{0x0FC63DAF, 0x8483, 0x4772, {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}};
    inline constexpr Guid LinuxSwap{0x0657FD6D, 0xA4AB, 0x43C4, {0x84, 0xE5, 0x09, 0x33, 0xC8, 0x4B, 0x4F, 0x4F}};
    inline constexpr Guid LinuxLVM{0xE6D6D379, 0xF507, 0x44C2, {0xA2, 0x3C, 0x23, 0x8F, 0x2A, 0x3D, 0xF9, 0x28}};
    inline constexpr Guid LinuxRAID{0xA19D880F, 0x05FC, 0x4D3B, {0xA0, 0x06, 0x74, 0x3F, 0x0F, 0x84, 0x91, 0x1E}};
    inline constexpr Guid LinuxHome{0x933AC7E1, 0x2EB4, 0x4F13, {0xB8, 0x44, 0x0E, 0x14, 0xE2, 0xAE, 0xF9, 0x15}};
    inline constexpr Guid AndroidBoot{0x49A4D17F, 0x93A3, 0x45C1, {0xA0, 0xDE, 0xF5, 0x0B, 0x2E, 0xBE, 0x25, 0x99}};
    inline constexpr Guid AndroidSystem{0xA40B6AED, 0x947B, 0x4A43, {0x87, 0x59, 0x69, 0x26, 0x7C, 0xE6, 0x67, 0x3F}};
    inline constexpr Guid AndroidMetadata{0x20AC5E2D, 0x1DDC, 0x47F2, {0x9D, 0x16, 0xD2, 0xB9, 0xFA, 0xB2, 0x55, 0xD5}};
    inline constexpr Guid AndroidUserdata{0x9158010C, 0xE1F2, 0x49D8, {0x9F, 0x8C, 0x1F, 0x14, 0x83, 0x0F, 0x0B, 0x96}};
    inline constexpr Guid AndroidPersist{0xE2790A88, 0x518E, 0x40E5, {0x8C, 0xA2, 0x7C, 0x7B, 0x1C, 0x1B, 0x07, 0xD5}};
    inline constexpr Guid QualcommDump{0xDEA217BA, 0x8C20, 0x47DF, {0x9C, 0xFE, 0x25, 0x2F, 0x0D, 0xD1, 0xB1, 0x0C}};
    inline constexpr Guid QualcommSSD{0x303E6AC3, 0x7F0F, 0x4C64, {0x93, 0x4C, 0x11, 0x01, 0xAF, 0xB5, 0x38, 0x9E}};
}

// ── Partition Attributes ──

enum class PartitionAttribute : uint64_t {
    RequiredPartition   = 0x0000000000000001ULL,
    NoBlockIOProtocol   = 0x0000000000000002ULL,
    LegacyBIOSBootable  = 0x0000000000000004ULL,
    TypeGUIDBit0        = 0x0001000000000000ULL,
    TypeGUIDBit1        = 0x0002000000000000ULL,
    TypeGUIDBit2        = 0x0004000000000000ULL,
    TypeGUIDBit3        = 0x0008000000000000ULL,
    AndroidShouldFlash  = 0x0001000000000000ULL,
    AndroidDontFlash    = 0x0002000000000000ULL,
    AndroidShouldBackup = 0x0004000000000000ULL,
};

constexpr PartitionAttribute operator|(PartitionAttribute a, PartitionAttribute b) noexcept {
    return static_cast<PartitionAttribute>(
        static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

constexpr bool hasAttribute(uint64_t attrs, PartitionAttribute flag) noexcept {
    return (attrs & static_cast<uint64_t>(flag)) != 0;
}

// ── GPT Layout ──

struct GPTLayout {
    uint64_t totalSectors{0};
    uint32_t sectorSize{512};
    uint64_t protectiveMBR{0};
    uint64_t primaryHeaderLBA{1};
    uint64_t primaryEntriesLBA{2};
    uint64_t firstUsableLBA{34};
    uint64_t lastUsableLBA{0};
    uint64_t backupEntriesLBA{0};
    uint64_t backupHeaderLBA{0};
    uint64_t alignment{2048};

    uint64_t firstUsableByte() const noexcept;
    uint64_t lastUsableByte() const noexcept;
    uint64_t totalUsableSectors() const noexcept;
    bool isValid() const noexcept;
};

// ── Partition Info (user-facing) ──

struct PartitionInfo {
    Guid typeGUID;
    Guid uniqueGUID;
    std::string name;
    uint64_t firstLBA{0};
    uint64_t lastLBA{0};
    uint64_t byteOffset{0};
    uint64_t byteLength{0};
    uint64_t attributes{0};
    bool isUsed{false};

    uint64_t sizeInSectors() const noexcept;
};

// ── Partition Range ──

struct PartitionRange {
    uint64_t firstLBA{0};
    uint64_t lastLBA{0};

    uint64_t sectorCount() const noexcept;
    uint64_t byteCount(uint32_t sectorSize = 512) const noexcept;
    bool contains(uint64_t lba) const noexcept;
    bool overlaps(const PartitionRange& other) const noexcept;
};

// ── GPT Table (parsed result) ──

struct GPTTable {
    GPTHeader primaryHeader;
    GPTHeader backupHeader;
    std::vector<PartitionEntry> entries;
    uint64_t entryCount{0};
    uint64_t entrySize{0};
    bool primaryValid{false};
    bool backupValid{false};
    bool entriesValid{false};
    bool hasValidProtectiveMBR{false};
    uint64_t diskSizeInSectors{0};
    uint32_t sectorSize{512};

    bool isValid() const noexcept;
    size_t usedEntryCount() const noexcept;
};

// ── Protective MBR Entry ──

struct ProtectiveMBREntry {
    uint8_t bootIndicator{0};
    uint8_t startingCHS[3]{};
    uint8_t partitionType{0};
    uint8_t endingCHS[3]{};
    uint32_t startingLBA{0};
    uint32_t sizeInLBA{0};

    bool isProtective() const noexcept;
};

}} // namespace mbootcore::gpt
