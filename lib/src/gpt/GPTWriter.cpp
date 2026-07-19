#include "mbootcore/gpt/GPTWriter.hpp"
#include <zlib.h>
#include <cstring>
#include <algorithm>
#include <limits>

namespace mbootcore {
namespace gpt {

namespace {

inline bool mulOverflows(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) return false;
    return a > std::numeric_limits<uint64_t>::max() / b;
}

inline bool safeMul(uint64_t a, uint64_t b, uint64_t& result) {
    if (mulOverflows(a, b)) return false;
    result = a * b;
    return true;
}

} // anonymous namespace

GPTWriter::GPTWriter(IFlashDevice& device)
    : m_device(device) {
    auto storageResult = m_device.getStorageInfo();
    if (storageResult) {
        m_layout.sectorSize = storageResult.value().sectorSize;
        m_layout.totalSectors = storageResult.value().numSectors;
    }
    if (m_layout.sectorSize == 0) m_layout.sectorSize = 512;

    m_layout.lastUsableLBA = m_layout.totalSectors > 34
        ? m_layout.totalSectors - 34 - 1 : 0;
    m_layout.backupEntriesLBA = m_layout.totalSectors > 33
        ? m_layout.totalSectors - 33 : 0;
    m_layout.backupHeaderLBA = m_layout.totalSectors > 0
        ? m_layout.totalSectors - 1 : 0;
}

uint64_t GPTWriter::lbaToByte(uint64_t lba) const noexcept {
    return lba * m_layout.sectorSize;
}

uint32_t GPTWriter::computeCRC32(const ByteBuffer& data) const {
    return ::crc32(0, data.data(), static_cast<uInt>(data.size()));
}

ByteBuffer GPTWriter::serializeHeader(const GPTHeader& header) const {
    ByteBuffer data(512, 0);

    auto writeLE16 = [&](size_t off, uint16_t val) {
        data[off]     = static_cast<uint8_t>(val);
        data[off + 1] = static_cast<uint8_t>(val >> 8);
    };
    auto writeLE32 = [&](size_t off, uint32_t val) {
        data[off]     = static_cast<uint8_t>(val);
        data[off + 1] = static_cast<uint8_t>(val >> 8);
        data[off + 2] = static_cast<uint8_t>(val >> 16);
        data[off + 3] = static_cast<uint8_t>(val >> 24);
    };
    auto writeLE64 = [&](size_t off, uint64_t val) {
        data[off]     = static_cast<uint8_t>(val);
        data[off + 1] = static_cast<uint8_t>(val >> 8);
        data[off + 2] = static_cast<uint8_t>(val >> 16);
        data[off + 3] = static_cast<uint8_t>(val >> 24);
        data[off + 4] = static_cast<uint8_t>(val >> 32);
        data[off + 5] = static_cast<uint8_t>(val >> 40);
        data[off + 6] = static_cast<uint8_t>(val >> 48);
        data[off + 7] = static_cast<uint8_t>(val >> 56);
    };

    writeLE64(0, header.signature);
    writeLE32(8, header.revision);
    writeLE32(12, header.headerSize);
    writeLE32(16, header.headerCrc32);
    writeLE32(20, header.reserved);
    writeLE64(24, header.myLBA);
    writeLE64(32, header.alternateLBA);
    writeLE64(40, header.firstUsableLBA);
    writeLE64(48, header.lastUsableLBA);

    writeLE32(56, header.diskGUID.data1);
    writeLE16(60, header.diskGUID.data2);
    writeLE16(62, header.diskGUID.data3);
    for (int i = 0; i < 8; ++i) {
        data[64 + i] = header.diskGUID.data4[i];
    }

    writeLE64(72, header.partitionEntryLBA);
    writeLE32(80, header.numberOfPartitionEntries);
    writeLE32(84, header.sizeOfPartitionEntry);
    writeLE32(88, header.partitionEntriesCRC32);

    return data;
}

ByteBuffer GPTWriter::serializeEntries(const std::vector<PartitionEntry>& entries,
                                        uint64_t count, uint64_t entrySize) const {
    size_t totalSize = static_cast<size_t>(count * entrySize);
    if (totalSize == 0) return {};
    ByteBuffer data(totalSize, 0);

    auto writeLE16 = [&](size_t off, uint16_t val) {
        data[off]     = static_cast<uint8_t>(val);
        data[off + 1] = static_cast<uint8_t>(val >> 8);
    };
    auto writeLE32 = [&](size_t off, uint32_t val) {
        data[off]     = static_cast<uint8_t>(val);
        data[off + 1] = static_cast<uint8_t>(val >> 8);
        data[off + 2] = static_cast<uint8_t>(val >> 16);
        data[off + 3] = static_cast<uint8_t>(val >> 24);
    };
    auto writeLE64 = [&](size_t off, uint64_t val) {
        data[off]     = static_cast<uint8_t>(val);
        data[off + 1] = static_cast<uint8_t>(val >> 8);
        data[off + 2] = static_cast<uint8_t>(val >> 16);
        data[off + 3] = static_cast<uint8_t>(val >> 24);
        data[off + 4] = static_cast<uint8_t>(val >> 32);
        data[off + 5] = static_cast<uint8_t>(val >> 40);
        data[off + 6] = static_cast<uint8_t>(val >> 48);
        data[off + 7] = static_cast<uint8_t>(val >> 56);
    };

    for (uint64_t i = 0; i < count && i < entries.size(); ++i) {
        size_t offset = static_cast<size_t>(i * entrySize);
        const auto& e = entries[i];

        writeLE32(offset, e.partitionTypeGUID.data1);
        writeLE16(offset + 4, e.partitionTypeGUID.data2);
        writeLE16(offset + 6, e.partitionTypeGUID.data3);
        std::copy(e.partitionTypeGUID.data4.begin(), e.partitionTypeGUID.data4.end(),
                  data.begin() + offset + 8);

        writeLE32(offset + 16, e.uniquePartitionGUID.data1);
        writeLE16(offset + 20, e.uniquePartitionGUID.data2);
        writeLE16(offset + 22, e.uniquePartitionGUID.data3);
        std::copy(e.uniquePartitionGUID.data4.begin(), e.uniquePartitionGUID.data4.end(),
                  data.begin() + offset + 24);

        writeLE64(offset + 32, e.firstLBA);
        writeLE64(offset + 40, e.lastLBA);
        writeLE64(offset + 48, e.attributes);

        for (int j = 0; j < 36; ++j) {
            writeLE16(offset + 56 + j * 2, static_cast<uint16_t>(e.name[j]));
        }
    }

    return data;
}

Result<void> GPTWriter::writeSector(uint64_t lba, const ByteBuffer& data) {
    uint64_t byteOffset = 0;
    if (!safeMul(lba, m_layout.sectorSize, byteOffset)) {
        return ErrorCode::InvalidArgument;
    }
    return m_device.writeMemory(byteOffset, data);
}

Result<void> GPTWriter::writeSectors(uint64_t lba, const ByteBuffer& data) {
    uint64_t byteOffset = 0;
    if (!safeMul(lba, m_layout.sectorSize, byteOffset)) {
        return ErrorCode::InvalidArgument;
    }
    return m_device.writeMemory(byteOffset, data);
}

Result<void> GPTWriter::writePrimaryHeader(const GPTHeader& header) {
    auto data = serializeHeader(header);
    return writeSector(m_layout.primaryHeaderLBA, data);
}

Result<void> GPTWriter::writeBackupHeader(const GPTHeader& header) {
    GPTHeader backupHdr = header;
    backupHdr.myLBA = m_layout.backupHeaderLBA;
    backupHdr.alternateLBA = m_layout.primaryHeaderLBA;
    backupHdr.partitionEntryLBA = m_layout.backupEntriesLBA;

    MBOOT_TRY(updateHeaderCRC(backupHdr));

    auto data = serializeHeader(backupHdr);
    return writeSector(m_layout.backupHeaderLBA, data);
}

Result<void> GPTWriter::writePartitionEntries(const GPTHeader& header,
                                               const std::vector<PartitionEntry>& entries) {
    auto data = serializeEntries(entries, header.numberOfPartitionEntries,
                                  header.sizeOfPartitionEntry);
    if (data.empty()) {
        return ErrorCode::InvalidArgument;
    }

    return writeSectors(header.partitionEntryLBA, data);
}

Result<void> GPTWriter::writePrimaryAndBackup(const GPTHeader& header,
                                               const std::vector<PartitionEntry>& entries) {
    MBOOT_TRY(writePartitionEntries(header, entries));
    MBOOT_TRY(writePrimaryHeader(header));
    MBOOT_TRY(writeBackupHeader(header));

    // Write backup entries
    auto backupData = serializeEntries(entries, header.numberOfPartitionEntries,
                                        header.sizeOfPartitionEntry);
    if (!backupData.empty()) {
        MBOOT_TRY(writeSectors(m_layout.backupEntriesLBA, backupData));
    }

    return {};
}

Result<void> GPTWriter::updateHeaderCRC(GPTHeader& header) {
    MBOOT_TRY_ASSIGN(crcVal, computeHeaderCRC(header));
    header.headerCrc32 = crcVal;
    return {};
}

Result<uint32_t> GPTWriter::computeHeaderCRC(const GPTHeader& header) const {
    if (header.headerSize < GPTHeader::MinHeaderSize) {
        return ErrorCode::InvalidArgument;
    }
    uint32_t hdrSize = std::min<uint32_t>(header.headerSize, 512u);
    ByteBuffer data = serializeHeader(header);
    if (data.size() < hdrSize) {
        return ErrorCode::InvalidArgument;
    }
    // Zero out CRC field
    data[16] = 0; data[17] = 0; data[18] = 0; data[19] = 0;

    ByteBuffer truncated(data.begin(), data.begin() + hdrSize);
    uint32_t crc = computeCRC32(truncated);
    return crc;
}

Result<uint32_t> GPTWriter::computeEntryCRC(
    const std::vector<PartitionEntry>& entries,
    uint64_t count, uint64_t entrySize) const {
    auto data = serializeEntries(entries, count, entrySize);
    if (data.empty()) {
        return 0;
    }
    uint32_t crc = computeCRC32(data);
    return crc;
}

GPTLayout GPTWriter::layout() const noexcept {
    return m_layout;
}

}} // namespace mbootcore::gpt
