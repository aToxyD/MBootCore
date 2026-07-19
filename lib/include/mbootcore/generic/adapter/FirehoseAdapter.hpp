#pragma once

#include "mbootcore/generic/IFlashDevice.hpp"
#include "mbootcore/domain/ILogger.hpp"
#include "mbootcore/core/protocols/firehose/FirehoseProtocol.hpp"

#include <memory>

namespace mbootcore {

class FirehoseAdapter : public IFlashDevice {
public:
    FirehoseAdapter(ITransport& transport, ILogger& logger);
    ~FirehoseAdapter() override = default;

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

    const FirehoseProtocol& protocol() const noexcept { return m_protocol; }
    FirehoseProtocol& protocol() noexcept { return m_protocol; }

private:
    Result<ByteBuffer> readImpl(uint64_t sectorSize, uint64_t numSectors);
    Result<void> writeImpl(uint64_t sectorSize, uint64_t numSectors,
                           const ByteBuffer& data);

    ITransport& m_transport;
    ILogger& m_logger;
    FirehoseProtocol m_protocol;
    bool m_open{false};
    ProgressCallback m_callback;
};

} // namespace mbootcore
