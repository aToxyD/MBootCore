#include "mbootcore/generic/adapter/SaharaAdapter.hpp"

namespace mbootcore {

SaharaAdapter::SaharaAdapter(ITransport& transport, ILogger& logger)
    : m_transport(transport)
    , m_logger(logger)
    , m_protocol(transport, logger) {}

Result<void> SaharaAdapter::open() {
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

void SaharaAdapter::close() noexcept {
    m_open = false;
    m_protocol.resetState();
    m_transport.close();
}

bool SaharaAdapter::isOpen() const noexcept {
    return m_open && m_transport.isOpen();
}

FlashCapability SaharaAdapter::capabilities() const noexcept {
    return FlashCapability::UploadLoader
         | FlashCapability::Reset;
}

Result<GenericDeviceInfo> SaharaAdapter::deviceInfo() {
    if (!m_open) return ErrorCode::DeviceDisconnected;

    const auto& info = m_protocol.deviceInfo();
    GenericDeviceInfo di;
    di.protocolName = "Sahara";
    di.protocolVersion = info.version.major;
    di.serialNumber = std::to_string(info.serialNumber);
    if (info.id.msmId > 0) {
        di.chipset = "MSM" + std::to_string(info.id.msmId);
    }
    di.bootMode = BootMode::EDL;
    return di;
}

Result<StorageInfo> SaharaAdapter::getStorageInfo() {
    return ErrorCode::NotSupported;
}

Result<PartitionTable> SaharaAdapter::getPartitions() {
    return ErrorCode::NotSupported;
}

Result<ByteBuffer> SaharaAdapter::readMemory(uint64_t, size_t) {
    return ErrorCode::NotSupported;
}

Result<void> SaharaAdapter::writeMemory(uint64_t, const ByteBuffer&) {
    return ErrorCode::NotSupported;
}

Result<void> SaharaAdapter::eraseMemory(uint64_t, size_t) {
    return ErrorCode::NotSupported;
}

Result<ByteBuffer> SaharaAdapter::readPartition(const std::string&) {
    return ErrorCode::NotSupported;
}

Result<void> SaharaAdapter::writePartition(const std::string&, const ByteBuffer&) {
    return ErrorCode::NotSupported;
}

Result<void> SaharaAdapter::erasePartition(const std::string&) {
    return ErrorCode::NotSupported;
}

Result<void> SaharaAdapter::uploadLoader(const ByteBuffer& programmerData) {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    return m_protocol.uploadProgrammer(programmerData);
}

Result<void> SaharaAdapter::reset() {
    if (!m_open) return ErrorCode::DeviceDisconnected;
    auto result = m_protocol.reset();
    if (result) m_open = false;
    return result;
}

Result<void> SaharaAdapter::powerReset() {
    return ErrorCode::NotSupported;
}

void SaharaAdapter::cancel() noexcept {
    m_protocol.cancel();
}

void SaharaAdapter::setProgressCallback(ProgressCallback callback) {
    if (callback) {
        m_protocol.onProgress(
            [cb = std::move(callback)](size_t current, size_t total) {
                ProgressInfo pi;
                pi.totalBytes = total;
                pi.transferredBytes = current;
                pi.percentage = total > 0 ? (100.0 * current / total) : 0.0;
                pi.currentOperation = "Uploading";
                pi.stage = "transfer";
                cb(pi);
            });
    }
}

} // namespace mbootcore
