#include "mbootcore/generic/adapter/FirehoseAdapter.hpp"

#include "SafeParser.hpp"

#include <cstring>
#include <limits>

namespace mbootcore {

FirehoseAdapter::FirehoseAdapter(ITransport& transport, ILogger& logger)
    : m_transport(transport)
    , m_logger(logger)
    , m_protocol(transport, logger) {}

Result<void> FirehoseAdapter::open() {
    if (m_open) return {};

    MBOOT_TRY(m_transport.open());

    auto result = m_protocol.handshake();
    if (!result.isOk()) {
        m_transport.close();
        return result;
    }

    m_open = true;
    return {};
}

void FirehoseAdapter::close() noexcept {
    m_open = false;
    m_protocol.resetState();
    m_transport.close();
}

bool FirehoseAdapter::isOpen() const noexcept {
    return m_open && m_transport.isOpen();
}

FlashCapability FirehoseAdapter::capabilities() const noexcept {
    return FlashCapability::Read
         | FlashCapability::Write
         | FlashCapability::Erase
         | FlashCapability::Reset
         | FlashCapability::PowerReset
         | FlashCapability::StorageInfo
         | FlashCapability::Peek
         | FlashCapability::Poke
         | FlashCapability::Patch
         | FlashCapability::Sha256Digest;
}

Result<GenericDeviceInfo> FirehoseAdapter::deviceInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    GenericDeviceInfo di;
    di.protocolName = "Firehose";
    di.bootMode = BootMode::Firehose;
    return di;
}

namespace {

StorageType storageTypeFromFirehose(const std::string& membType) noexcept {
    if (membType == "UFS" || membType == "ufs") return StorageType::UFS;
    if (membType == "eMMC" || membType == "emmc") return StorageType::eMMC;
    if (membType == "NAND" || membType == "nand") return StorageType::NAND;
    if (membType == "NOR" || membType == "nor") return StorageType::NOR;
    if (membType == "SPI" || membType == "spi") return StorageType::SPI;
    return StorageType::Unknown;
}

bool safeMul(uint64_t a, uint64_t b, uint64_t& result) {
    if (a != 0 && b > std::numeric_limits<uint64_t>::max() / a) return false;
    result = a * b;
    return true;
}

bool safeAdd(uint64_t a, uint64_t b, uint64_t& result) {
    if (b > std::numeric_limits<uint64_t>::max() - a) return false;
    result = a + b;
    return true;
}

uint64_t parseStorageSize(const XmlElement& root) {
    for (const auto& child : root.children) {
        if (child.name == "storage_info") {
            for (const auto& infoChild : child.children) {
                if (infoChild.name == "lun_info") {
                    for (const auto& lunChild : infoChild.children) {
                        if (lunChild.name == "size") {
                            auto r = fromCharsUint64(lunChild.content);
                            return r.ok ? r.value : 0;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

} // anonymous namespace

Result<StorageInfo> FirehoseAdapter::getStorageInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    MBOOT_TRY_ASSIGN(storageResult, m_protocol.getStorageInfo());

    StorageInfo si;
    si.type = StorageType::UFS;
    si.name = m_protocol.config().memoryName;

    auto parsed = FirehoseXmlEngine::parse(storageResult.rawXml());
    if (parsed) {
        auto& root = parsed.value();
        for (const auto& child : root.children) {
            if (child.name == "storage_info") {
                for (const auto& infoChild : child.children) {
                    if (infoChild.name == "memb_type") {
                        si.type = storageTypeFromFirehose(infoChild.content);
                    }
                }
            }
        }
        si.capacity = parseStorageSize(root);
    }

    si.sectorSize = 4096;
    if (si.capacity > 0) {
        si.numSectors = si.capacity;
    }

    return si;
}

Result<PartitionTable> FirehoseAdapter::getPartitions() {
    return ErrorCode::NotSupported;
}

namespace {

struct SectorMapping {
    uint64_t startSector;
    uint32_t sectorSize;
    uint64_t numSectors;
    uint64_t offset;
};

SectorMapping byteAddressToSectors(uint64_t byteAddress, size_t byteSize,
                                    uint32_t sectorSize) {
    SectorMapping sm{};
    sm.sectorSize = sectorSize;
    if (sectorSize == 0) return sm;

    sm.startSector = byteAddress / sectorSize;
    sm.offset = byteAddress % sectorSize;

    // size_t is at most 64 bits on all supported platforms (x86_64, ARM64),
    // so widening to uint64_t is safe.
    uint64_t endByte = 0;
    if (!safeAdd(byteAddress, static_cast<uint64_t>(byteSize), endByte)) {
        return sm;
    }

    // Round endByte up to the next sector boundary.
    uint64_t roundedEnd = 0;
    if (!safeAdd(endByte, static_cast<uint64_t>(sectorSize - 1), roundedEnd)) {
        return sm;
    }

    uint64_t endSector = roundedEnd / sectorSize;
    sm.numSectors = endSector - sm.startSector;
    return sm;
}

} // anonymous namespace

Result<ByteBuffer> FirehoseAdapter::readMemory(uint64_t address, size_t size) {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    uint32_t sectorSize = 4096;
    auto sm = byteAddressToSectors(address, size, sectorSize);

    if (sm.startSector > std::numeric_limits<uint32_t>::max() ||
        sm.numSectors > std::numeric_limits<uint32_t>::max()) {
        return ErrorCode::InvalidArgument;
    }

    ReadCommand cmd;
    cmd.startSector = static_cast<uint32_t>(sm.startSector);
    cmd.sectorSize = sm.sectorSize;
    cmd.numSectorSize = static_cast<uint32_t>(sm.numSectors);

    MBOOT_TRY_ASSIGN(raw, m_protocol.read(cmd));

    if (sm.offset > 0 || raw.size() > size) {
        ByteBuffer trimmed(raw.begin() + static_cast<ptrdiff_t>(sm.offset),
                           raw.begin() + static_cast<ptrdiff_t>(sm.offset + size));
        return trimmed;
    }

    return raw;
}

Result<void> FirehoseAdapter::writeMemory(uint64_t address, const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    uint32_t sectorSize = 4096;
    auto sm = byteAddressToSectors(address, data.size(), sectorSize);

    if (sm.startSector > std::numeric_limits<uint32_t>::max() ||
        sm.numSectors > std::numeric_limits<uint32_t>::max()) {
        return ErrorCode::InvalidArgument;
    }

    uint64_t totalAlloc64 = 0;
    if (!safeMul(sm.numSectors, static_cast<uint64_t>(sectorSize), totalAlloc64) ||
        totalAlloc64 > std::numeric_limits<size_t>::max()) {
        return ErrorCode::InvalidArgument;
    }

    ProgramCommand cmd;
    cmd.startSector = static_cast<uint32_t>(sm.startSector);
    cmd.sectorSize = sm.sectorSize;
    cmd.numSectorSize = static_cast<uint32_t>(sm.numSectors);

    ByteBuffer alignedData(static_cast<size_t>(totalAlloc64), 0xFF);
    std::memcpy(alignedData.data() + sm.offset, data.data(), data.size());

    MBOOT_TRY(m_protocol.program(cmd, alignedData));
    return {};
}

Result<void> FirehoseAdapter::eraseMemory(uint64_t address, size_t size) {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    uint32_t sectorSize = 4096;
    auto sm = byteAddressToSectors(address, size, sectorSize);

    if (sm.startSector > std::numeric_limits<uint32_t>::max() ||
        sm.numSectors > std::numeric_limits<uint32_t>::max()) {
        return ErrorCode::InvalidArgument;
    }

    EraseCommand cmd;
    cmd.startSector = static_cast<uint32_t>(sm.startSector);
    cmd.sectorSize = sm.sectorSize;
    cmd.numSectorSize = static_cast<uint32_t>(sm.numSectors);
    return m_protocol.erase(cmd);
}

Result<ByteBuffer> FirehoseAdapter::readPartition(const std::string& name) {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    ReadCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 0x1000;
    cmd.partitionId = 0;
    m_logger.info("Firehose", "Reading partition: " + name);
    return m_protocol.read(cmd);
}

Result<void> FirehoseAdapter::writePartition(const std::string& name,
                                              const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    ProgramCommand cmd;
    cmd.filename = name;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = static_cast<uint32_t>(data.size() / 4096 + 1);
    cmd.partitionId = 0;

    MBOOT_TRY(m_protocol.program(cmd, data));
    return {};
}

Result<void> FirehoseAdapter::erasePartition(const std::string& name) {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    EraseCommand cmd;
    cmd.filename = name;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 0x1000;
    cmd.partitionId = 0;
    m_logger.info("Firehose", "Erasing partition: " + name);
    return m_protocol.erase(cmd);
}

Result<void> FirehoseAdapter::uploadLoader(const ByteBuffer&) {
    return ErrorCode::NotSupported;
}

Result<void> FirehoseAdapter::reset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    auto result = m_protocol.reset();
    if (result) m_open = false;
    return result;
}

Result<void> FirehoseAdapter::powerReset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    PowerCommand cmd;
    return m_protocol.powerReset(cmd);
}

void FirehoseAdapter::cancel() noexcept {
    m_protocol.cancel();
}

void FirehoseAdapter::setProgressCallback(ProgressCallback callback) {
    m_callback = std::move(callback);
    if (m_callback) {
        m_protocol.onProgress(
            [cb = m_callback](size_t current, size_t total) {
                ProgressInfo pi;
                pi.totalBytes = total;
                pi.transferredBytes = current;
                pi.percentage = total > 0 ? (100.0 * current / total) : 0.0;
                pi.currentOperation = "Transferring";
                pi.stage = "stream";
                cb(pi);
            });
    }
}

} // namespace mbootcore
