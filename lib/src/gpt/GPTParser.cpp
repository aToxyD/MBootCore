#include "mbootcore/gpt/GPTParser.hpp"
#include <zlib.h>
#include <cstring>
#include <algorithm>
#include <limits>
#include "common/NumericUtils.hpp"

namespace mbootcore {
namespace gpt {

namespace {

// ── Validation limits ──
// These bounds reject corrupted/malicious GPT headers before any allocation.
//
// kMaxPartitionEntries: 4096 entries. GPT spec commonly uses 128.
//   4096 exceeds typical real-world layouts while preventing DoS via
//   excessive allocation requests from a crafted header.
//
// kMinEntrySize: 128 bytes. The GPT specification defines partition entries
//   as 128 bytes. Smaller values are always invalid.
//
// kMaxEntrySize: 512 bytes. Conservative upper bound. Larger values are
//   unheard of in practice and would indicate corruption.
constexpr uint32_t kMaxPartitionEntries = 4096;
constexpr uint32_t kMinEntrySize = 128;
constexpr uint32_t kMaxEntrySize = 512;

// ── Overflow-safe arithmetic ──

// Returns true if a * b would overflow uint64_t.
inline bool mulOverflows(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) return false;
    return a > std::numeric_limits<uint64_t>::max() / b;
}

// Safe multiply: returns false on overflow.
inline bool safeMul(uint64_t a, uint64_t b, uint64_t& result) {
    if (mulOverflows(a, b)) return false;
    result = a * b;
    return true;
}

// ── Header field validation ──
// Validates numberOfPartitionEntries and sizeOfPartitionEntry immediately
// after decoding, before any allocation, reservation, or sector read
// derived from those values.
inline bool validateEntryFields(uint32_t numEntries, uint32_t entrySize,
                                std::vector<ErrorCode>& warnings) {
    if (numEntries == 0 || numEntries > kMaxPartitionEntries) {
        warnings.push_back(ErrorCode::GPTInvalidHeader);
        return false;
    }
    if (entrySize < kMinEntrySize || entrySize > kMaxEntrySize) {
        warnings.push_back(ErrorCode::GPTInvalidHeader);
        return false;
    }
    return true;
}

} // anonymous namespace

GPTParser::GPTParser(IFlashDevice& device)
    : m_device(device) {}

uint64_t GPTParser::lbaToByte(uint64_t lba) const noexcept {
    return lba * m_layout.sectorSize;
}

uint64_t GPTParser::byteToLBA(uint64_t byteAddr) const noexcept {
    return byteAddr / m_layout.sectorSize;
}

ByteBuffer GPTParser::readSector(uint64_t lba) {
    uint64_t byteOffset = 0;
    if (!safeMul(lba, m_layout.sectorSize, byteOffset)) {
        return ByteBuffer{};
    }
    auto result = m_device.readMemory(byteOffset, m_layout.sectorSize);
    if (result.isError()) return ByteBuffer{};
    return result.value();
}

ByteBuffer GPTParser::readSectors(uint64_t lba, uint64_t count) {
    uint64_t bytes = 0;
    if (!safeMul(m_layout.sectorSize, count, bytes)) {
        return ByteBuffer{};
    }
    // Prevent LBA × sectorSize overflow before converting to byte offset.
    uint64_t byteOffset = 0;
    if (!safeMul(lba, m_layout.sectorSize, byteOffset)) {
        return ByteBuffer{};
    }
    auto result = m_device.readMemory(byteOffset, numeric::checked_cast<size_t>(bytes));
    if (result.isError()) return ByteBuffer{};
    return result.value();
}

uint32_t GPTParser::computeCRC32(const ByteBuffer& data) const {
    return ::crc32(0, data.data(), static_cast<uInt>(data.size()));
}

ProtectiveMBREntry GPTParser::readProtectiveMBR(const ByteBuffer& sector) {
    ProtectiveMBREntry entry{};
    if (sector.size() < 512) return entry;

    // MBR signature at bytes 510-511
    uint16_t signature = static_cast<uint16_t>(sector[510]) |
                        (static_cast<uint16_t>(sector[511]) << 8);
    if (signature != 0xAA55) return entry;

    // First partition entry at offset 0x1BE (446)
    constexpr size_t kPartEntryOffset = 446;
    if (sector.size() < kPartEntryOffset + 16) return entry;

    entry.bootIndicator = sector[kPartEntryOffset];
    entry.startingCHS[0] = sector[kPartEntryOffset + 1];
    entry.startingCHS[1] = sector[kPartEntryOffset + 2];
    entry.startingCHS[2] = sector[kPartEntryOffset + 3];
    entry.partitionType = sector[kPartEntryOffset + 4];
    entry.endingCHS[0] = sector[kPartEntryOffset + 5];
    entry.endingCHS[1] = sector[kPartEntryOffset + 6];
    entry.endingCHS[2] = sector[kPartEntryOffset + 7];
    entry.startingLBA = static_cast<uint32_t>(sector[kPartEntryOffset + 8]) |
                       (static_cast<uint32_t>(sector[kPartEntryOffset + 9]) << 8) |
                       (static_cast<uint32_t>(sector[kPartEntryOffset + 10]) << 16) |
                       (static_cast<uint32_t>(sector[kPartEntryOffset + 11]) << 24);
    entry.sizeInLBA = static_cast<uint32_t>(sector[kPartEntryOffset + 12]) |
                     (static_cast<uint32_t>(sector[kPartEntryOffset + 13]) << 8) |
                     (static_cast<uint32_t>(sector[kPartEntryOffset + 14]) << 16) |
                     (static_cast<uint32_t>(sector[kPartEntryOffset + 15]) << 24);

    return entry;
}

GPTHeader GPTParser::readHeader(const ByteBuffer& data, uint64_t lba) {
    (void)lba;
    GPTHeader header{};
    if (data.size() < 92) return header;

    auto readLE16 = [&](size_t off) -> uint16_t {
        return static_cast<uint16_t>(data[off]) |
              (static_cast<uint16_t>(data[off + 1]) << 8);
    };
    auto readLE32 = [&](size_t off) -> uint32_t {
        return static_cast<uint32_t>(data[off]) |
              (static_cast<uint32_t>(data[off + 1]) << 8) |
              (static_cast<uint32_t>(data[off + 2]) << 16) |
              (static_cast<uint32_t>(data[off + 3]) << 24);
    };
    auto readLE64 = [&](size_t off) -> uint64_t {
        return static_cast<uint64_t>(data[off]) |
              (static_cast<uint64_t>(data[off + 1]) << 8) |
              (static_cast<uint64_t>(data[off + 2]) << 16) |
              (static_cast<uint64_t>(data[off + 3]) << 24) |
              (static_cast<uint64_t>(data[off + 4]) << 32) |
              (static_cast<uint64_t>(data[off + 5]) << 40) |
              (static_cast<uint64_t>(data[off + 6]) << 48) |
              (static_cast<uint64_t>(data[off + 7]) << 56);
    };

    header.signature = readLE64(0);
    header.revision = readLE32(8);
    header.headerSize = readLE32(12);
    header.headerCrc32 = readLE32(16);
    header.reserved = readLE32(20);
    header.myLBA = readLE64(24);
    header.alternateLBA = readLE64(32);
    header.firstUsableLBA = readLE64(40);
    header.lastUsableLBA = readLE64(48);

    header.diskGUID.data1 = readLE32(56);
    header.diskGUID.data2 = readLE16(60);
    header.diskGUID.data3 = readLE16(62);
    for (int i = 0; i < 8; ++i) {
        header.diskGUID.data4[i] = data[64 + i];
    }

    header.partitionEntryLBA = readLE64(72);
    header.numberOfPartitionEntries = readLE32(80);
    header.sizeOfPartitionEntry = readLE32(84);
    header.partitionEntriesCRC32 = readLE32(88);

    return header;
}

std::vector<PartitionEntry> GPTParser::readEntries(const ByteBuffer& data,
                                                     uint64_t count,
                                                     uint64_t entrySize) {
    std::vector<PartitionEntry> entries;
    entries.reserve(numeric::checked_cast<size_t>(count));

    auto readLE16 = [&](size_t off) -> uint16_t {
        if (off + 1 >= data.size()) return 0;
        return static_cast<uint16_t>(data[off]) |
              (static_cast<uint16_t>(data[off + 1]) << 8);
    };
    auto readLE32 = [&](size_t off) -> uint32_t {
        if (off + 3 >= data.size()) return 0;
        return static_cast<uint32_t>(data[off]) |
              (static_cast<uint32_t>(data[off + 1]) << 8) |
              (static_cast<uint32_t>(data[off + 2]) << 16) |
              (static_cast<uint32_t>(data[off + 3]) << 24);
    };
    auto readLE64 = [&](size_t off) -> uint64_t {
        if (off + 7 >= data.size()) return 0;
        return static_cast<uint64_t>(data[off]) |
              (static_cast<uint64_t>(data[off + 1]) << 8) |
              (static_cast<uint64_t>(data[off + 2]) << 16) |
              (static_cast<uint64_t>(data[off + 3]) << 24) |
              (static_cast<uint64_t>(data[off + 4]) << 32) |
              (static_cast<uint64_t>(data[off + 5]) << 40) |
              (static_cast<uint64_t>(data[off + 6]) << 48) |
              (static_cast<uint64_t>(data[off + 7]) << 56);
    };

    for (uint64_t i = 0; i < count; ++i) {
        uint64_t offset64 = 0;
        if (!safeMul(i, entrySize, offset64)) break;
        size_t offset = static_cast<size_t>(offset64);
        if (offset + entrySize > data.size()) break;

        PartitionEntry entry{};
        entry.partitionTypeGUID.data1 = readLE32(offset);
        entry.partitionTypeGUID.data2 = readLE16(offset + 4);
        entry.partitionTypeGUID.data3 = readLE16(offset + 6);
        for (int j = 0; j < 8; ++j) {
            entry.partitionTypeGUID.data4[j] = data[offset + 8 + j];
        }

        entry.uniquePartitionGUID.data1 = readLE32(offset + 16);
        entry.uniquePartitionGUID.data2 = readLE16(offset + 20);
        entry.uniquePartitionGUID.data3 = readLE16(offset + 22);
        for (int j = 0; j < 8; ++j) {
            entry.uniquePartitionGUID.data4[j] = data[offset + 24 + j];
        }

        entry.firstLBA = readLE64(offset + 32);
        entry.lastLBA = readLE64(offset + 40);
        entry.attributes = readLE64(offset + 48);

        for (int j = 0; j < 36; ++j) {
            uint16_t code = readLE16(offset + 56 + j * 2);
            entry.name[j] = static_cast<char16_t>(code);
        }

        entries.push_back(entry);
    }

    return entries;
}

uint32_t GPTParser::computeEntryCRC32(const std::vector<PartitionEntry>& entries,
                                       uint64_t count, uint64_t entrySize) const {
    uint64_t totalBytes = 0;
    if (!safeMul(count, entrySize, totalBytes)) return 0;
    ByteBuffer raw(static_cast<size_t>(totalBytes), 0);
    for (uint64_t i = 0; i < count && i < entries.size(); ++i) {
        uint64_t offset64 = 0;
        if (!safeMul(i, entrySize, offset64)) break;
        size_t offset = static_cast<size_t>(offset64);
        const auto& e = entries[numeric::checked_cast<size_t>(i)];
        auto writeLE32 = [&](size_t off, uint32_t val) {
            raw[off]       = static_cast<uint8_t>(val);
            raw[off + 1]   = static_cast<uint8_t>(val >> 8);
            raw[off + 2]   = static_cast<uint8_t>(val >> 16);
            raw[off + 3]   = static_cast<uint8_t>(val >> 24);
        };
        auto writeLE16 = [&](size_t off, uint16_t val) {
            raw[off]       = static_cast<uint8_t>(val);
            raw[off + 1]   = static_cast<uint8_t>(val >> 8);
        };
        auto writeLE64 = [&](size_t off, uint64_t val) {
            raw[off]       = static_cast<uint8_t>(val);
            raw[off + 1]   = static_cast<uint8_t>(val >> 8);
            raw[off + 2]   = static_cast<uint8_t>(val >> 16);
            raw[off + 3]   = static_cast<uint8_t>(val >> 24);
            raw[off + 4]   = static_cast<uint8_t>(val >> 32);
            raw[off + 5]   = static_cast<uint8_t>(val >> 40);
            raw[off + 6]   = static_cast<uint8_t>(val >> 48);
            raw[off + 7]   = static_cast<uint8_t>(val >> 56);
        };

        writeLE32(offset, e.partitionTypeGUID.data1);
        writeLE16(offset + 4, e.partitionTypeGUID.data2);
        writeLE16(offset + 6, e.partitionTypeGUID.data3);
        std::copy(e.partitionTypeGUID.data4.begin(), e.partitionTypeGUID.data4.end(),
                  raw.begin() + offset + 8);

        writeLE32(offset + 16, e.uniquePartitionGUID.data1);
        writeLE16(offset + 20, e.uniquePartitionGUID.data2);
        writeLE16(offset + 22, e.uniquePartitionGUID.data3);
        std::copy(e.uniquePartitionGUID.data4.begin(), e.uniquePartitionGUID.data4.end(),
                  raw.begin() + offset + 24);

        writeLE64(offset + 32, e.firstLBA);
        writeLE64(offset + 40, e.lastLBA);
        writeLE64(offset + 48, e.attributes);

        for (int j = 0; j < 36; ++j) {
            writeLE16(offset + 56 + j * 2, static_cast<uint16_t>(e.name[j]));
        }
    }
    return computeCRC32(raw);
}

ParserResult GPTParser::parse() {
    ParserResult result;
    m_hasValidMBR = false;

    // Determine sector size from device
    auto storageResult = m_device.getStorageInfo();
    if (storageResult.isOk()) {
        m_layout.sectorSize = storageResult.value().sectorSize;
        m_layout.totalSectors = storageResult.value().numSectors;
    } else {
        m_layout.sectorSize = 512;
    }
    if (m_layout.sectorSize == 0) m_layout.sectorSize = 512;

    // Read protective MBR (LBA 0)
    auto mbrData = readSector(0);
    if (mbrData.empty()) {
        result.errors.push_back(ErrorCode::TransportReadFailed);
        return result;
    }

    auto mbr = readProtectiveMBR(mbrData);
    m_hasValidMBR = mbr.isProtective();
    result.table.hasValidProtectiveMBR = m_hasValidMBR;
    result.table.diskSizeInSectors = m_layout.totalSectors;
    result.table.sectorSize = m_layout.sectorSize;

    // Compute layout
    m_layout.lastUsableLBA = m_layout.totalSectors > 34
        ? m_layout.totalSectors - 34 - 1 : 0;
    m_layout.backupEntriesLBA = m_layout.totalSectors > 34
        ? m_layout.totalSectors - 33 : 0;
    m_layout.backupHeaderLBA = m_layout.totalSectors > 0
        ? m_layout.totalSectors - 1 : 0;

    // Read and validate primary GPT header (LBA 1)
    result.table.primaryValid = false;
    auto primaryData = readSector(m_layout.primaryHeaderLBA);
    if (primaryData.size() >= 92) {
        result.table.primaryHeader = readHeader(primaryData, m_layout.primaryHeaderLBA);

        if (result.table.primaryHeader.hasValidSignature() &&
            result.table.primaryHeader.headerSize >= GPTHeader::MinHeaderSize) {

            // Validate header CRC
            ByteBuffer headerCopy = primaryData;
            // Zero the CRC field (bytes 16-19)
            headerCopy[16] = 0; headerCopy[17] = 0;
            headerCopy[18] = 0; headerCopy[19] = 0;

            uint32_t hdrSize = std::min<uint32_t>(
                result.table.primaryHeader.headerSize, 512);
            ByteBuffer truncated(headerCopy.begin(),
                                  headerCopy.begin() + hdrSize);
            uint32_t computedCRC = computeCRC32(truncated);

            if (computedCRC == result.table.primaryHeader.headerCrc32) {
                result.table.primaryValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        } else {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
        }
    } else {
        result.warnings.push_back(ErrorCode::GPTPrimaryMissing);
    }

    // Read and validate backup GPT header
    result.table.backupValid = false;
    if (m_layout.backupHeaderLBA > 0) {
        auto backupData = readSector(m_layout.backupHeaderLBA);
        if (backupData.size() >= 92) {
            result.table.backupHeader = readHeader(backupData, m_layout.backupHeaderLBA);

            if (result.table.backupHeader.hasValidSignature() &&
                result.table.backupHeader.headerSize >= GPTHeader::MinHeaderSize) {

                ByteBuffer headerCopy = backupData;
                headerCopy[16] = 0; headerCopy[17] = 0;
                headerCopy[18] = 0; headerCopy[19] = 0;

                uint32_t hdrSize = std::min<uint32_t>(
                    result.table.backupHeader.headerSize, 512);
                ByteBuffer truncated(headerCopy.begin(),
                                      headerCopy.begin() + hdrSize);
                uint32_t computedCRC = computeCRC32(truncated);

                if (computedCRC == result.table.backupHeader.headerCrc32) {
                    result.table.backupValid = true;
                } else {
                    result.warnings.push_back(ErrorCode::GPTCrcMismatch);
                }
            } else {
                result.warnings.push_back(ErrorCode::GPTInvalidHeader);
            }
        } else {
            result.warnings.push_back(ErrorCode::GPTBackupMissing);
        }
    }

    // Read partition entries
    result.table.entriesValid = false;
    if (result.table.primaryValid) {
        auto& hdr = result.table.primaryHeader;

        if (!validateEntryFields(hdr.numberOfPartitionEntries,
                                 hdr.sizeOfPartitionEntry,
                                 result.warnings)) {
            result.table.primaryValid = false;
            return result;
        }

        uint64_t entryCount = hdr.numberOfPartitionEntries;
        uint64_t entrySize = hdr.sizeOfPartitionEntry;
        uint64_t totalEntryBytes = 0;
        if (!safeMul(entryCount, entrySize, totalEntryBytes)) {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
            result.table.primaryValid = false;
            return result;
        }
        uint64_t entrySectors = (totalEntryBytes + m_layout.sectorSize - 1)
                                / m_layout.sectorSize;

        auto entryData = readSectors(hdr.partitionEntryLBA, entrySectors);

        if (entryData.size() >= totalEntryBytes) {
            result.table.entries = readEntries(entryData, entryCount, entrySize);
            result.table.entryCount = entryCount;
            result.table.entrySize = entrySize;

            // Validate entry CRC
            uint32_t computedEntryCRC = computeEntryCRC32(
                result.table.entries, entryCount, entrySize);

            if (computedEntryCRC == hdr.partitionEntriesCRC32) {
                result.table.entriesValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        } else {
            result.warnings.push_back(ErrorCode::GPTInvalidEntry);
        }
    } else if (result.table.backupValid) {
        // Fallback to backup header for partition entries
        auto& hdr = result.table.backupHeader;

        if (!validateEntryFields(hdr.numberOfPartitionEntries,
                                 hdr.sizeOfPartitionEntry,
                                 result.warnings)) {
            result.table.backupValid = false;
            return result;
        }

        uint64_t entryCount = hdr.numberOfPartitionEntries;
        uint64_t entrySize = hdr.sizeOfPartitionEntry;
        uint64_t totalEntryBytes = 0;
        if (!safeMul(entryCount, entrySize, totalEntryBytes)) {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
            result.table.backupValid = false;
            return result;
        }
        uint64_t entrySectors = (totalEntryBytes + m_layout.sectorSize - 1)
                                / m_layout.sectorSize;

        auto entryData = readSectors(hdr.partitionEntryLBA, entrySectors);

        if (entryData.size() >= totalEntryBytes) {
            result.table.entries = readEntries(entryData, entryCount, entrySize);
            result.table.entryCount = entryCount;
            result.table.entrySize = entrySize;

            uint32_t computedEntryCRC = computeEntryCRC32(
                result.table.entries, entryCount, entrySize);

            if (computedEntryCRC == hdr.partitionEntriesCRC32) {
                result.table.entriesValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        }
    }

    // Update layout from parsed header
    if (result.table.primaryValid) {
        m_layout.firstUsableLBA = result.table.primaryHeader.firstUsableLBA;
        m_layout.lastUsableLBA = result.table.primaryHeader.lastUsableLBA;
    }

    return result;
}

ParserResult GPTParser::parsePrimary() {
    ParserResult result;
    m_hasValidMBR = false;

    auto storageResult = m_device.getStorageInfo();
    if (storageResult.isOk()) {
        m_layout.sectorSize = storageResult.value().sectorSize;
        m_layout.totalSectors = storageResult.value().numSectors;
    }
    if (m_layout.sectorSize == 0) m_layout.sectorSize = 512;

    auto mbrData = readSector(0);
    if (mbrData.empty()) {
        result.errors.push_back(ErrorCode::TransportReadFailed);
        return result;
    }

    auto mbr = readProtectiveMBR(mbrData);
    m_hasValidMBR = mbr.isProtective();
    result.table.hasValidProtectiveMBR = m_hasValidMBR;

    auto primaryData = readSector(m_layout.primaryHeaderLBA);
    if (primaryData.size() >= 92) {
        result.table.primaryHeader = readHeader(primaryData, m_layout.primaryHeaderLBA);

        if (result.table.primaryHeader.hasValidSignature() &&
            result.table.primaryHeader.headerSize >= GPTHeader::MinHeaderSize) {

            ByteBuffer headerCopy = primaryData;
            headerCopy[16] = 0; headerCopy[17] = 0;
            headerCopy[18] = 0; headerCopy[19] = 0;

            uint32_t hdrSize = std::min<uint32_t>(
                result.table.primaryHeader.headerSize, 512);
            ByteBuffer truncated(headerCopy.begin(),
                                  headerCopy.begin() + hdrSize);
            uint32_t computedCRC = computeCRC32(truncated);

            if (computedCRC == result.table.primaryHeader.headerCrc32) {
                result.table.primaryValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        } else {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
        }
    } else {
        result.warnings.push_back(ErrorCode::GPTPrimaryMissing);
    }

    if (result.table.primaryValid) {
        auto& hdr = result.table.primaryHeader;

        if (!validateEntryFields(hdr.numberOfPartitionEntries,
                                 hdr.sizeOfPartitionEntry,
                                 result.warnings)) {
            result.table.primaryValid = false;
            return result;
        }

        uint64_t entryCount = hdr.numberOfPartitionEntries;
        uint64_t entrySize = hdr.sizeOfPartitionEntry;
        uint64_t totalEntryBytes = 0;
        if (!safeMul(entryCount, entrySize, totalEntryBytes)) {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
            result.table.primaryValid = false;
            return result;
        }
        uint64_t entrySectors = (totalEntryBytes + m_layout.sectorSize - 1)
                                / m_layout.sectorSize;

        auto entryData = readSectors(hdr.partitionEntryLBA, entrySectors);

        if (entryData.size() >= totalEntryBytes) {
            result.table.entries = readEntries(entryData, entryCount, entrySize);
            result.table.entryCount = entryCount;
            result.table.entrySize = entrySize;

            uint32_t computedEntryCRC = computeEntryCRC32(
                result.table.entries, entryCount, entrySize);

            if (computedEntryCRC == hdr.partitionEntriesCRC32) {
                result.table.entriesValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        } else {
            result.warnings.push_back(ErrorCode::GPTInvalidEntry);
        }
    }

    return result;
}

ParserResult GPTParser::parseBackup() {
    ParserResult result;

    auto storageResult = m_device.getStorageInfo();
    if (storageResult.isOk()) {
        m_layout.sectorSize = storageResult.value().sectorSize;
        m_layout.totalSectors = storageResult.value().numSectors;
    }
    if (m_layout.sectorSize == 0) m_layout.sectorSize = 512;

    m_layout.backupHeaderLBA = m_layout.totalSectors > 0
        ? m_layout.totalSectors - 1 : 0;
    m_layout.backupEntriesLBA = m_layout.totalSectors > 33
        ? m_layout.totalSectors - 33 : 0;

    if (m_layout.backupHeaderLBA == 0) {
        result.errors.push_back(ErrorCode::GPTBackupMissing);
        return result;
    }

    auto backupData = readSector(m_layout.backupHeaderLBA);
    if (backupData.size() >= 92) {
        result.table.backupHeader = readHeader(backupData, m_layout.backupHeaderLBA);

        if (result.table.backupHeader.hasValidSignature() &&
            result.table.backupHeader.headerSize >= GPTHeader::MinHeaderSize) {

            ByteBuffer headerCopy = backupData;
            headerCopy[16] = 0; headerCopy[17] = 0;
            headerCopy[18] = 0; headerCopy[19] = 0;

            uint32_t hdrSize = std::min<uint32_t>(
                result.table.backupHeader.headerSize, 512);
            ByteBuffer truncated(headerCopy.begin(),
                                  headerCopy.begin() + hdrSize);
            uint32_t computedCRC = computeCRC32(truncated);

            if (computedCRC == result.table.backupHeader.headerCrc32) {
                result.table.backupValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        } else {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
        }
    } else {
        result.warnings.push_back(ErrorCode::GPTBackupMissing);
    }

    if (result.table.backupValid) {
        auto& hdr = result.table.backupHeader;

        if (!validateEntryFields(hdr.numberOfPartitionEntries,
                                 hdr.sizeOfPartitionEntry,
                                 result.warnings)) {
            result.table.backupValid = false;
            return result;
        }

        uint64_t entryCount = hdr.numberOfPartitionEntries;
        uint64_t entrySize = hdr.sizeOfPartitionEntry;
        uint64_t totalEntryBytes = 0;
        if (!safeMul(entryCount, entrySize, totalEntryBytes)) {
            result.warnings.push_back(ErrorCode::GPTInvalidHeader);
            result.table.backupValid = false;
            return result;
        }
        uint64_t entrySectors = (totalEntryBytes + m_layout.sectorSize - 1)
                                / m_layout.sectorSize;

        auto entryData = readSectors(hdr.partitionEntryLBA, entrySectors);

        if (entryData.size() >= totalEntryBytes) {
            result.table.entries = readEntries(entryData, entryCount, entrySize);
            result.table.entryCount = entryCount;
            result.table.entrySize = entrySize;

            uint32_t computedEntryCRC = computeEntryCRC32(
                result.table.entries, entryCount, entrySize);

            if (computedEntryCRC == hdr.partitionEntriesCRC32) {
                result.table.entriesValid = true;
            } else {
                result.warnings.push_back(ErrorCode::GPTCrcMismatch);
            }
        }
    }

    return result;
}

bool GPTParser::hasValidProtectiveMBR() const noexcept {
    return m_hasValidMBR;
}

const GPTLayout& GPTParser::layout() const noexcept {
    return m_layout;
}

// ── ProtectiveMBREntry ──

bool ProtectiveMBREntry::isProtective() const noexcept {
    return partitionType == 0xEE;
}

// ── GPTHeader methods ──

bool GPTHeader::isValid() const noexcept {
    return hasValidSignature() &&
           headerSize >= MinHeaderSize &&
           revision == Revision &&
           myLBA > 0 &&
           alternateLBA > 0 &&
           firstUsableLBA < lastUsableLBA &&
           numberOfPartitionEntries > 0 &&
           sizeOfPartitionEntry >= MinEntrySize;
}

bool GPTHeader::hasValidSignature() const noexcept {
    return signature == Signature;
}

uint64_t GPTHeader::totalEntrySize() const noexcept {
    return static_cast<uint64_t>(numberOfPartitionEntries) *
           static_cast<uint64_t>(sizeOfPartitionEntry);
}

uint64_t GPTHeader::lastEntryLBA() const noexcept {
    uint64_t totalBytes = totalEntrySize();
    uint64_t sectorsForEntries = (totalBytes + 511) / 512;
    return partitionEntryLBA + sectorsForEntries - 1;
}

// ── PartitionEntry methods ──

bool PartitionEntry::isUsed() const noexcept {
    return !partitionTypeGUID.isNull();
}

bool PartitionEntry::isValid() const noexcept {
    return isUsed() && firstLBA <= lastLBA && firstLBA > 0;
}

uint64_t PartitionEntry::sizeInLBAs() const noexcept {
    if (lastLBA <= firstLBA) return 0;
    return lastLBA - firstLBA + 1;
}

uint64_t PartitionEntry::sizeInBytes() const noexcept {
    uint64_t lbas = sizeInLBAs();
    constexpr uint64_t kSectorSize = 512;
    if (lbas > std::numeric_limits<uint64_t>::max() / kSectorSize) return 0;
    return lbas * kSectorSize;
}

std::string PartitionEntry::nameToString() const {
    size_t len = 0;
    while (len < 36 && name[len] != 0) ++len;
    return utf16ToUtf8(name.data(), len);
}

std::u16string PartitionEntry::utf8ToUtf16(std::string_view utf8) {
    std::u16string result;
    for (size_t i = 0; i < utf8.size(); ) {
        uint32_t cp = 0;
        uint8_t b = static_cast<uint8_t>(utf8[i]);
        if (b < 0x80) {
            cp = b; i += 1;
        } else if (b < 0xE0) {
            cp = (b & 0x1F) << 6;
            if (i + 1 < utf8.size()) cp |= (utf8[i + 1] & 0x3F);
            i += 2;
        } else if (b < 0xF0) {
            cp = (b & 0x0F) << 12;
            if (i + 2 < utf8.size()) {
                cp |= (utf8[i + 1] & 0x3F) << 6;
                cp |= (utf8[i + 2] & 0x3F);
            }
            i += 3;
        } else {
            cp = (b & 0x07) << 18;
            if (i + 3 < utf8.size()) {
                cp |= (utf8[i + 1] & 0x3F) << 12;
                cp |= (utf8[i + 2] & 0x3F) << 6;
                cp |= (utf8[i + 3] & 0x3F);
            }
            i += 4;
        }
        if (cp <= 0xFFFF) {
            result.push_back(static_cast<char16_t>(cp));
        } else {
            cp -= 0x10000;
            result.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
            result.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
        }
    }
    return result;
}

std::string PartitionEntry::utf16ToUtf8(const char16_t* utf16, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i) {
        uint32_t cp = utf16[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len) {
            uint32_t low = utf16[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }
        if (cp < 0x80) {
            result.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return result;
}

// ── GPTTable methods ──

bool GPTTable::isValid() const noexcept {
    return primaryValid || backupValid;
}

size_t GPTTable::usedEntryCount() const noexcept {
    return std::count_if(entries.begin(), entries.end(),
        [](const PartitionEntry& e) { return e.isUsed(); });
}

// ── GPTLayout methods ──

uint64_t GPTLayout::firstUsableByte() const noexcept {
    return firstUsableLBA * sectorSize;
}

uint64_t GPTLayout::lastUsableByte() const noexcept {
    return lastUsableLBA * sectorSize;
}

uint64_t GPTLayout::totalUsableSectors() const noexcept {
    if (lastUsableLBA <= firstUsableLBA) return 0;
    return lastUsableLBA - firstUsableLBA + 1;
}

bool GPTLayout::isValid() const noexcept {
    return totalSectors > 0 && sectorSize > 0 &&
           firstUsableLBA < lastUsableLBA;
}

// ── PartitionInfo methods ──

uint64_t PartitionInfo::sizeInSectors() const noexcept {
    if (lastLBA <= firstLBA) return 0;
    return lastLBA - firstLBA + 1;
}

// ── PartitionRange methods ──

uint64_t PartitionRange::sectorCount() const noexcept {
    if (lastLBA <= firstLBA) return 0;
    return lastLBA - firstLBA + 1;
}

uint64_t PartitionRange::byteCount(uint32_t sectorSize) const noexcept {
    uint64_t sectors = sectorCount();
    if (sectorSize != 0 &&
        sectors > std::numeric_limits<uint64_t>::max() / sectorSize) return 0;
    return sectors * sectorSize;
}

bool PartitionRange::contains(uint64_t lba) const noexcept {
    return lba >= firstLBA && lba <= lastLBA;
}

bool PartitionRange::overlaps(const PartitionRange& other) const noexcept {
    return firstLBA <= other.lastLBA && lastLBA >= other.firstLBA;
}

}} // namespace mbootcore::gpt
