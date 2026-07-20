#include "mbootcore/gpt/PartitionManager.hpp"
#include <algorithm>
#include <limits>
#include "common/NumericUtils.hpp"

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

PartitionManager::PartitionManager(IFlashDevice& device)
    : m_device(device)
    , m_parser(device)
    , m_writer(device) {}

Result<void> PartitionManager::open() {
    auto result = m_device.open();
    if (result) {
        m_open = true;
        MBOOT_TRY(refreshTable());
    }
    return result;
}

Result<GPTTable> PartitionManager::refreshTable() {
    if (!m_open) {
        MBOOT_TRY(m_device.open());
        m_open = true;
    }

    auto parserResult = m_parser.parse();
    m_table = parserResult.table;
    m_layout = m_parser.layout();

    if (!parserResult.errors.empty()) {
        return parserResult.errors.front();
    }

    return m_table;
}

Result<PartitionInfo> PartitionManager::entryToInfo(const PartitionEntry& entry) const {
    PartitionInfo info;
    info.typeGUID = entry.partitionTypeGUID;
    info.uniqueGUID = entry.uniquePartitionGUID;
    info.name = entry.nameToString();
    info.firstLBA = entry.firstLBA;
    info.lastLBA = entry.lastLBA;
    uint64_t byteOffset = 0;
    if (!safeMul(entry.firstLBA, m_layout.sectorSize, byteOffset)) {
        byteOffset = 0;
    }
    info.byteOffset = byteOffset;
    info.byteLength = entry.sizeInBytes();
    info.attributes = entry.attributes;
    info.isUsed = entry.isUsed();
    return info;
}

Result<std::vector<PartitionInfo>> PartitionManager::listPartitions() {
    if (m_table.entries.empty()) {
        MBOOT_TRY(refreshTable());
    }

    std::vector<PartitionInfo> partitions;
    for (const auto& entry : m_table.entries) {
        if (entry.isUsed()) {
            partitions.push_back(entryToInfo(entry).value());
        }
    }
    return partitions;
}

Result<PartitionInfo> PartitionManager::findByName(const std::string& name) {
    if (m_table.entries.empty()) {
        MBOOT_TRY(refreshTable());
    }

    for (const auto& entry : m_table.entries) {
        if (entry.isUsed()) {
            auto entryName = entry.nameToString();
            if (entryName == name) {
                return entryToInfo(entry).value();
            }
        }
    }
    return ErrorCode::PartitionNotFound;
}

Result<PartitionInfo> PartitionManager::findByGUID(const Guid& guid) {
    if (m_table.entries.empty()) {
        MBOOT_TRY(refreshTable());
    }

    for (const auto& entry : m_table.entries) {
        if (entry.isUsed() && entry.uniquePartitionGUID == guid) {
            return entryToInfo(entry).value();
        }
    }
    return ErrorCode::PartitionNotFound;
}

Result<std::vector<PartitionInfo>> PartitionManager::findByType(const Guid& typeGUID) {
    if (m_table.entries.empty()) {
        MBOOT_TRY(refreshTable());
    }

    std::vector<PartitionInfo> matches;
    for (const auto& entry : m_table.entries) {
        if (entry.partitionTypeGUID == typeGUID) {
            matches.push_back(entryToInfo(entry).value());
        }
    }
    return matches;
}

bool PartitionManager::exists(const std::string& name) {
    auto result = findByName(name);
    return static_cast<bool>(result);
}

size_t PartitionManager::partitionCount() const noexcept {
    return m_table.usedEntryCount();
}

Result<ByteBuffer> PartitionManager::readPartition(const std::string& name) {
    MBOOT_TRY_ASSIGN(info, findByName(name));

    return m_device.readMemory(info.byteOffset, numeric::checked_cast<size_t>(info.byteLength));
}

Result<void> PartitionManager::writePartition(const std::string& name,
                                               const ByteBuffer& data) {
    MBOOT_TRY_ASSIGN(info, findByName(name));

    if (data.size() > info.byteLength) {
        return ErrorCode::ImageTooLarge;
    }

    return m_device.writeMemory(info.byteOffset, data);
}

Result<void> PartitionManager::erasePartition(const std::string& name) {
    MBOOT_TRY_ASSIGN(info, findByName(name));

    return m_device.eraseMemory(info.byteOffset, numeric::checked_cast<size_t>(info.byteLength));
}

Result<void> PartitionManager::trimPartition(const std::string& name) {
    MBOOT_TRY_ASSIGN(info, findByName(name));

    return m_device.eraseMemory(info.byteOffset, numeric::checked_cast<size_t>(info.byteLength));
}

Result<ByteBuffer> PartitionManager::backupPartition(const std::string& name) {
    return readPartition(name);
}

Result<void> PartitionManager::restorePartition(const std::string& name,
                                                  const ByteBuffer& data) {
    return writePartition(name, data);
}

Result<bool> PartitionManager::verifyPartition(const std::string& name,
                                                const ByteBuffer& expected) {
    MBOOT_TRY_ASSIGN(actual, readPartition(name));

    if (actual.size() != expected.size()) {
        return false;
    }

    bool match = std::equal(actual.begin(), actual.end(), expected.begin());
    return match;
}

Result<bool> PartitionManager::comparePartition(const std::string& name,
                                                  const ByteBuffer& data) {
    return verifyPartition(name, data);
}

Result<void> PartitionManager::recoverFromBackup() {
    auto parserResult = m_parser.parse();
    m_table = parserResult.table;

    if (m_table.primaryValid && m_table.backupValid) {
        return {};
    }

    if (m_table.backupValid && !m_table.primaryValid) {
        // Recovery: backup → primary
        auto& backup = m_table.backupHeader;
        GPTHeader newPrimary = backup;
        newPrimary.myLBA = 1;
        newPrimary.alternateLBA = m_layout.backupHeaderLBA;
        newPrimary.partitionEntryLBA = 2;
        newPrimary.firstUsableLBA = backup.firstUsableLBA;
        newPrimary.lastUsableLBA = backup.lastUsableLBA;

        MBOOT_TRY_ASSIGN(crcVal, m_writer.computeHeaderCRC(newPrimary));
        newPrimary.headerCrc32 = crcVal;

        // Compute entry CRC
        auto entryCRC = m_writer.computeEntryCRC(
            m_table.entries, newPrimary.numberOfPartitionEntries,
            newPrimary.sizeOfPartitionEntry);
        if (entryCRC) {
            newPrimary.partitionEntriesCRC32 = entryCRC.value();
        }

        MBOOT_TRY(m_writer.writePartitionEntries(newPrimary, m_table.entries));
        MBOOT_TRY(m_writer.writePrimaryHeader(newPrimary));

        // Refresh
        MBOOT_TRY(refreshTable());
        return {};
    }

    if (m_table.primaryValid && !m_table.backupValid) {
        // Recovery: primary → backup
        auto& primary = m_table.primaryHeader;
        MBOOT_TRY(m_writer.writeBackupHeader(primary));

        // Write backup entries
        GPTHeader backupHdr = primary;
        backupHdr.myLBA = m_layout.backupHeaderLBA;
        backupHdr.partitionEntryLBA = m_layout.backupEntriesLBA;

        MBOOT_TRY(m_writer.writePartitionEntries(backupHdr, m_table.entries));

        // Update primary alternateLBA if needed
        GPTHeader updatedPrimary = primary;
        updatedPrimary.alternateLBA = m_layout.backupHeaderLBA;
        MBOOT_TRY(m_writer.updateHeaderCRC(updatedPrimary));
        MBOOT_TRY(m_writer.writePrimaryHeader(updatedPrimary));

        MBOOT_TRY(refreshTable());
        return {};
    }

    return ErrorCode::GPTCorrupted;
}

const GPTTable& PartitionManager::table() const noexcept {
    return m_table;
}

const GPTLayout& PartitionManager::layout() const noexcept {
    return m_layout;
}

bool PartitionManager::isOpen() const noexcept {
    return m_open;
}

void PartitionManager::setProgressCallback(ProgressCallback callback) {
    m_callback = std::move(callback);
}

}} // namespace mbootcore::gpt
