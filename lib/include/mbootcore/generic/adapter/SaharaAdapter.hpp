#pragma once

#include "mbootcore/generic/IFlashDevice.hpp"
#include "mbootcore/domain/ILogger.hpp"
#include "mbootcore/core/protocols/sahara/SaharaProtocol.hpp"

#include <memory>

namespace mbootcore {

class SaharaAdapter : public IFlashDevice {
public:
    SaharaAdapter(ITransport& transport, ILogger& logger);
    ~SaharaAdapter() override = default;

    // IFlashDevice
    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override;

    FlashCapability capabilities() const noexcept override;

    Result<GenericDeviceInfo> deviceInfo() override;
    Result<StorageInfo> getStorageInfo() override;
    Result<PartitionTable> getPartitions() override;

    Result<ByteBuffer> readMemory(uint64_t address, size_t size) override;
    Result<void> writeMemory(uint64_t address, const ByteBuffer& data) override;
    Result<void> eraseMemory(uint64_t address, size_t size) override;

    Result<ByteBuffer> readPartition(const std::string& name) override;
    Result<void> writePartition(const std::string& name,
                                const ByteBuffer& data) override;
    Result<void> erasePartition(const std::string& name) override;

    Result<void> uploadLoader(const ByteBuffer& programmerData) override;
    Result<void> reset() override;
    Result<void> powerReset() override;

    void cancel() noexcept override;
    void setProgressCallback(ProgressCallback callback) override;

    const SaharaProtocol& protocol() const noexcept { return m_protocol; }
    SaharaProtocol& protocol() noexcept { return m_protocol; }

private:
    ITransport& m_transport;
    [[maybe_unused]] ILogger& m_logger;
    SaharaProtocol m_protocol;
    bool m_open{false};
};

} // namespace mbootcore
