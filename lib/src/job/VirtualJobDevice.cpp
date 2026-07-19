#include <mbootcore/job/VirtualJobDevice.hpp>

#include <algorithm>
#include <cstring>

namespace mbootcore {
namespace job {

VirtualJobDevice::VirtualJobDevice()
    : m_storage(1024 * 1024, 0)
    , m_storageSize(1024 * 1024)
    , m_rng(m_rules.seed) {
    m_deviceInfo.vendor = "Virtual";
    m_deviceInfo.name = "VirtualJobDevice";
    m_deviceInfo.protocolName = "Virtual";
}

VirtualJobDevice::VirtualJobDevice(uint64_t storageSize)
    : m_storage(static_cast<size_t>(storageSize), 0)
    , m_storageSize(storageSize)
    , m_rng(m_rules.seed) {
    m_deviceInfo.vendor = "Virtual";
    m_deviceInfo.name = "VirtualJobDevice";
    m_deviceInfo.protocolName = "Virtual";
}

void VirtualJobDevice::setStorageSize(uint64_t bytes) {
    m_storage.assign(static_cast<size_t>(bytes), 0);
    m_storageSize = bytes;
}

void VirtualJobDevice::clearStorage() {
    std::fill(m_storage.begin(), m_storage.end(), 0);
}

void VirtualJobDevice::fillStorage(uint8_t value) {
    std::fill(m_storage.begin(), m_storage.end(), value);
}

void VirtualJobDevice::writeToStorage(uint64_t offset, const ByteBuffer& data) {
    if (offset + data.size() > m_storage.size()) {
        m_storage.resize(static_cast<size_t>(offset + data.size()), 0);
        m_storageSize = m_storage.size();
    }
    std::copy(data.begin(), data.end(),
              m_storage.begin() + static_cast<ptrdiff_t>(offset));
}

ByteBuffer VirtualJobDevice::readFromStorage(uint64_t offset, size_t size) const {
    if (offset + size > m_storage.size()) {
        size = m_storage.size() > static_cast<size_t>(offset)
               ? m_storage.size() - static_cast<size_t>(offset) : 0;
    }
    return ByteBuffer(m_storage.begin() + static_cast<ptrdiff_t>(offset),
                      m_storage.begin() + static_cast<ptrdiff_t>(offset + size));
}

void VirtualJobDevice::setTimeouts(std::chrono::milliseconds readDelay,
                                    std::chrono::milliseconds writeDelay,
                                    std::chrono::milliseconds eraseDelay) {
    m_readDelay = readDelay;
    m_writeDelay = writeDelay;
    m_eraseDelay = eraseDelay;
}

void VirtualJobDevice::resetCounters() {
    m_openCount = 0;
    m_closeCount = 0;
    m_readCount = 0;
    m_writeCount = 0;
    m_eraseCount = 0;
}

void VirtualJobDevice::addPartition(const std::string& name, uint64_t offset, uint64_t size) {
    PartInfo pi;
    pi.name = name;
    pi.offset = offset;
    pi.size = size;
    m_partitions.push_back(pi);
}

void VirtualJobDevice::clearPartitions() {
    m_partitions.clear();
}

Result<void> VirtualJobDevice::open() {
    if (m_rules.failOpen) {
        return ErrorCode::JobFailed;
    }
    m_open = true;
    m_openCount++;
    return {};
}

void VirtualJobDevice::close() noexcept {
    m_open = false;
    m_closeCount++;
}

Result<GenericDeviceInfo> VirtualJobDevice::deviceInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failGetInfo) return ErrorCode::JobFailed;
    if (m_rules.badGPT) {
        GenericDeviceInfo info = m_deviceInfo;
        info.name = "Corrupted Device";
        return info;
    }
    return m_deviceInfo;
}

Result<StorageInfo> VirtualJobDevice::getStorageInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    StorageInfo si;
    si.type = StorageType::eMMC;
    si.name = "virtual";
    si.numSectors = m_storageSize / 512;
    si.sectorSize = 512;
    si.capacity = m_storageSize;
    return si;
}

Result<PartitionTable> VirtualJobDevice::getPartitions() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failGetPartitions || m_rules.badGPT) {
        return ErrorCode::GPTCorrupted;
    }

    PartitionTable pt;
    pt.type = "GPT";
    pt.entryCount = static_cast<uint32_t>(m_partitions.size());
    pt.firstUsableLba = 34;
    pt.lastUsableLba = m_storageSize / 512 - 34;

    for (const auto& p : m_partitions) {
        PartitionEntry entry;
        entry.name = p.name;
        entry.byteOffset = p.offset;
        entry.byteLength = p.size;
        entry.startSector = p.offset / 512;
        entry.length = p.size / 512;
        pt.entries.push_back(entry);
    }

    return pt;
}

Result<ByteBuffer> VirtualJobDevice::readMemory(uint64_t address, size_t size) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failRead) return ErrorCode::JobFailed;
    if (m_rules.timeoutOnRead) {
        simulateDelay(m_readDelay);
        if (m_cancelled) return ErrorCode::Cancelled;
    }
    if (shouldRandomFail()) return ErrorCode::TransportError;

    if (address + size > m_storage.size()) {
        return ErrorCode::InvalidArgument;
    }

    m_readCount++;
    if (!m_rules.timeoutOnRead) {
        simulateDelay(m_readDelay);
    }
    reportProgress(address + size, m_storageSize, "Reading", "read");

    return ByteBuffer(m_storage.begin() + static_cast<ptrdiff_t>(address),
                   m_storage.begin() + static_cast<ptrdiff_t>(address + size));
}

Result<void> VirtualJobDevice::writeMemory(uint64_t address, const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failWrite) return ErrorCode::JobFailed;
    if (m_rules.timeoutOnWrite) {
        simulateDelay(m_writeDelay);
        if (m_cancelled) return ErrorCode::Cancelled;
    }
    if (m_rules.disconnectOnWrite) {
        m_open = false;
        return ErrorCode::DeviceDisconnected;
    }
    if (shouldRandomFail()) return ErrorCode::TransportError;

    if (address + data.size() > m_storage.size()) {
        return ErrorCode::InvalidArgument;
    }

    m_writeCount++;
    if (!m_rules.timeoutOnWrite) {
        simulateDelay(m_writeDelay);
    }

    if (m_rules.corruptData) {
        ByteBuffer corrupted = data;
        if (!corrupted.empty()) corrupted[0] ^= 0xFF;
        std::copy(corrupted.begin(), corrupted.end(),
                  m_storage.begin() + static_cast<ptrdiff_t>(address));
    } else {
        std::copy(data.begin(), data.end(),
                  m_storage.begin() + static_cast<ptrdiff_t>(address));
    }

    reportProgress(address + data.size(), m_storageSize, "Writing", "write");
    return {};
}

Result<void> VirtualJobDevice::eraseMemory(uint64_t address, size_t size) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failErase) return ErrorCode::JobFailed;
    if (m_rules.timeoutOnErase) {
        simulateDelay(m_eraseDelay);
        if (m_cancelled) return ErrorCode::Cancelled;
    }
    if (shouldRandomFail()) return ErrorCode::TransportError;

    if (address + size > m_storage.size()) {
        return ErrorCode::InvalidArgument;
    }

    m_eraseCount++;
    if (!m_rules.timeoutOnErase) {
        simulateDelay(m_eraseDelay);
    }
    std::fill(m_storage.begin() + static_cast<ptrdiff_t>(address),
              m_storage.begin() + static_cast<ptrdiff_t>(address + size), 0);

    reportProgress(address + size, m_storageSize, "Erasing", "erase");
    return {};
}

Result<ByteBuffer> VirtualJobDevice::readPartition(const std::string& name) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failRead) return ErrorCode::JobFailed;
    if (m_rules.timeoutOnRead) {
        simulateDelay(m_readDelay);
        if (m_cancelled) return ErrorCode::Cancelled;
    }
    if (m_rules.disconnectOnRead) {
        m_open = false;
        return ErrorCode::DeviceDisconnected;
    }

    auto part = findPartition(name);
    if (part.isError()) {
        return part.error();
    }

    m_readCount++;
    if (!m_rules.timeoutOnRead) {
        simulateDelay(m_readDelay);
    }
    auto data = readFromStorage(part.value().offset, static_cast<size_t>(part.value().size));

    reportProgress(part.value().size, part.value().size, "Reading", "read");
    return data;
}

Result<void> VirtualJobDevice::writePartition(const std::string& name,
                                              const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failWrite) return ErrorCode::JobFailed;
    if (m_rules.timeoutOnWrite) {
        simulateDelay(m_writeDelay);
        if (m_cancelled) return ErrorCode::Cancelled;
    }
    if (m_rules.disconnectOnWrite) {
        m_open = false;
        return ErrorCode::DeviceDisconnected;
    }

    auto part = findPartition(name);
    if (part.isError()) {
        return part.error();
    }

    if (data.size() > part.value().size) {
        return ErrorCode::JobInsufficientSpace;
    }

    m_writeCount++;
    if (!m_rules.timeoutOnWrite) {
        simulateDelay(m_writeDelay);
    }

    if (m_rules.corruptData) {
        ByteBuffer corrupted = data;
        if (!corrupted.empty()) corrupted[0] ^= 0xFF;
        writeToStorage(part.value().offset, corrupted);
    } else {
        writeToStorage(part.value().offset, data);
    }

    reportProgress(data.size(), part.value().size, "Writing", "write");
    return {};
}

Result<void> VirtualJobDevice::erasePartition(const std::string& name) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failErase) return ErrorCode::JobFailed;

    auto part = findPartition(name);
    if (part.isError()) {
        return part.error();
    }

    m_eraseCount++;
    simulateDelay(m_eraseDelay);
    std::fill(m_storage.begin() + static_cast<ptrdiff_t>(part.value().offset),
              m_storage.begin() + static_cast<ptrdiff_t>(part.value().offset + part.value().size),
              0);

    reportProgress(part.value().size, part.value().size, "Erasing", "erase");
    return {};
}

Result<void> VirtualJobDevice::uploadLoader(const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failUpload) return ErrorCode::JobFailed;
    reportProgress(data.size(), data.size(), "Uploading", "transfer");
    return {};
}

Result<void> VirtualJobDevice::reset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failReset) return ErrorCode::JobFailed;
    return {};
}

Result<void> VirtualJobDevice::powerReset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_rules.failReset) return ErrorCode::JobFailed;
    m_open = false;
    return {};
}

bool VirtualJobDevice::shouldRandomFail() const {
    if (m_rules.randomFailureRate <= 0.0) return false;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_rng) < m_rules.randomFailureRate;
}

void VirtualJobDevice::simulateDelay(std::chrono::milliseconds delay) const {
    if (delay.count() > 0 && !m_cancelled) {
        std::this_thread::sleep_for(delay);
    }
}

void VirtualJobDevice::reportProgress(uint64_t current, uint64_t total,
                                       const std::string& op,
                                       const std::string& stage) {
    if (m_progressCb) {
        ProgressInfo pi;
        pi.totalBytes = total;
        pi.transferredBytes = current;
        pi.percentage = total > 0 ? (100.0 * current / total) : 0.0;
        pi.currentOperation = op;
        pi.stage = stage;
        m_progressCb(pi);
    }
}

Result<VirtualJobDevice::PartInfo> VirtualJobDevice::findPartition(const std::string& name) const {
    for (const auto& p : m_partitions) {
        if (p.name == name) return p;
    }
    return ErrorCode::PartitionNotFound;
}

} // namespace job
} // namespace mbootcore
