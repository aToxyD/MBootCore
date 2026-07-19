#pragma once

#include "mbootcore/generic/IFlashDevice.hpp"
#include "mbootcore/generic/ProgressInfo.hpp"
#include "../mocks/MockTransport.hpp"
#include "../mocks/MockLogger.hpp"

#include <memory>

namespace mbootcore {

class VirtualFlashDevice : public IFlashDevice {
public:
    enum class Mode {
        SaharaOnly,
        FirehoseOnly,
        FullChain
    };

    explicit VirtualFlashDevice(ITransport& transport,
                                ILogger& logger,
                                Mode mode = Mode::FullChain);

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

private:
    ITransport& m_transport;
    ILogger& m_logger;
    Mode m_mode;
    GenericDeviceInfo m_info;
    FlashCapability m_caps{FlashCapability::All};
    StorageType m_storageType{StorageType::UFS};
    bool m_open{false};
    bool m_loaded{false};
    bool m_firehoseMode{false};
    ProgressCallback m_callback;
    std::vector<uint8_t> m_memory;

    void reportProgress(uint64_t current, uint64_t total,
                        const std::string& op, const std::string& stage);
};

} // namespace mbootcore
