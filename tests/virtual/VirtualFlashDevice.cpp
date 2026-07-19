#include "VirtualFlashDevice.hpp"

namespace mbootcore {

VirtualFlashDevice::VirtualFlashDevice(ITransport& transport,
                                       ILogger& logger,
                                       Mode mode)
    : m_transport(transport)
    , m_logger(logger)
    , m_mode(mode) {
    m_info.protocolName = "Virtual";
    m_info.bootMode = BootMode::EDL;
}

Result<void> VirtualFlashDevice::open() {
    if (m_open) return {};

    auto result = m_transport.open();
    if (!result.isOk()) return result;

    m_open = true;
    m_logger.info("VirtualFlash", "Device opened");

    if (m_mode == Mode::FirehoseOnly) {
        m_firehoseMode = true;
        m_memory.resize(16 * 1024 * 1024, uint8_t{0xFF});
    }

    return {};
}

void VirtualFlashDevice::close() noexcept {
    m_open = false;
    m_firehoseMode = false;
    m_loaded = false;
    m_transport.close();
}

bool VirtualFlashDevice::isOpen() const noexcept {
    return m_open;
}

FlashCapability VirtualFlashDevice::capabilities() const noexcept {
    return m_caps;
}

Result<GenericDeviceInfo> VirtualFlashDevice::deviceInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    return m_info;
}

Result<StorageInfo> VirtualFlashDevice::getStorageInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;

    StorageInfo si;
    si.type = m_storageType;
    si.name = "virtual";
    si.numSectors = 0x100000;
    si.sectorSize = 4096;
    si.capacity = si.numSectors * si.sectorSize;
    return si;
}

Result<PartitionTable> VirtualFlashDevice::getPartitions() {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    PartitionTable pt;
    pt.type = "GPT";
    pt.entryCount = 2;
    pt.firstUsableLba = 34;
    pt.lastUsableLba = 0xFFFFCC;
    return pt;
}

Result<ByteBuffer> VirtualFlashDevice::readMemory(uint64_t address, size_t size) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;
    if (address + size > m_memory.size()) return ErrorCode::InvalidArgument;
    ByteBuffer result(m_memory.begin() + static_cast<std::ptrdiff_t>(address),
                      m_memory.begin() + static_cast<std::ptrdiff_t>(address + size));
    return result;
}

Result<void> VirtualFlashDevice::writeMemory(uint64_t address, const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;
    if (address + data.size() > m_memory.size()) return ErrorCode::InvalidArgument;
    std::copy(data.begin(), data.end(),
              m_memory.begin() + static_cast<std::ptrdiff_t>(address));
    return {};
}

Result<void> VirtualFlashDevice::eraseMemory(uint64_t address, size_t size) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;
    if (address + size > m_memory.size()) return ErrorCode::InvalidArgument;
    std::fill(m_memory.begin() + static_cast<std::ptrdiff_t>(address),
              m_memory.begin() + static_cast<std::ptrdiff_t>(address + size),
              uint8_t{0xFF});
    return {};
}

Result<ByteBuffer> VirtualFlashDevice::readPartition(const std::string&) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;

    reportProgress(0, 100, "Reading", "read");
    ByteBuffer data(256, 0xA5);
    reportProgress(100, 100, "Reading", "complete");
    return data;
}

Result<void> VirtualFlashDevice::writePartition(const std::string&,
                                                 const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;

    reportProgress(0, data.size(), "Writing", "write");
    reportProgress(data.size(), data.size(), "Writing", "complete");
    return {};
}

Result<void> VirtualFlashDevice::erasePartition(const std::string&) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;
    return {};
}

Result<void> VirtualFlashDevice::uploadLoader(const ByteBuffer& data) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (m_firehoseMode) return ErrorCode::NotSupported;

    reportProgress(0, data.size(), "Uploading", "transfer");
    m_loaded = true;
    m_logger.info("VirtualFlash", "Loader uploaded");
    reportProgress(data.size(), data.size(), "Uploading", "complete");
    return {};
}

Result<void> VirtualFlashDevice::reset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    m_logger.info("VirtualFlash", "Reset to Firehose mode");
    m_firehoseMode = true;
    return {};
}

Result<void> VirtualFlashDevice::powerReset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    if (!m_firehoseMode) return ErrorCode::NotSupported;

    m_logger.info("VirtualFlash", "Power reset");
    return {};
}

void VirtualFlashDevice::cancel() noexcept {
    m_logger.info("VirtualFlash", "Cancelled");
}

void VirtualFlashDevice::setProgressCallback(ProgressCallback callback) {
    m_callback = std::move(callback);
}

void VirtualFlashDevice::reportProgress(uint64_t current, uint64_t total,
                                         const std::string& op,
                                         const std::string& stage) {
    if (m_callback) {
        ProgressInfo pi;
        pi.totalBytes = total;
        pi.transferredBytes = current;
        pi.percentage = total > 0 ? (100.0 * current / total) : 0.0;
        pi.currentOperation = op;
        pi.stage = stage;
        m_callback(pi);
    }
}

} // namespace mbootcore
