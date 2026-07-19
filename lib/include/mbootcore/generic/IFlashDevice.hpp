#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/generic/FlashCapability.hpp"
#include "mbootcore/generic/DeviceInfo.hpp"
#include "mbootcore/generic/StorageInfo.hpp"
#include "mbootcore/generic/PartitionModel.hpp"
#include "mbootcore/generic/ProgressInfo.hpp"

#include <string>
#include <vector>
#include <functional>

namespace mbootcore {

class IFlashDevice {
public:
    virtual ~IFlashDevice() = default;

    // Lifecycle
    virtual Result<void> open() = 0;
    virtual void close() noexcept = 0;
    virtual bool isOpen() const noexcept = 0;

    // Capabilities
    virtual FlashCapability capabilities() const noexcept = 0;

    // Device info
    virtual Result<GenericDeviceInfo> deviceInfo() = 0;

    // Storage info
    virtual Result<StorageInfo> getStorageInfo() = 0;
    virtual Result<PartitionTable> getPartitions() = 0;

    // Memory operations
    virtual Result<ByteBuffer> readMemory(uint64_t address, size_t size) = 0;
    virtual Result<void> writeMemory(uint64_t address, const ByteBuffer& data) = 0;
    virtual Result<void> eraseMemory(uint64_t address, size_t size) = 0;

    // Storage operations
    virtual Result<ByteBuffer> readPartition(const std::string& name) = 0;
    virtual Result<void> writePartition(const std::string& name,
                                        const ByteBuffer& data) = 0;
    virtual Result<void> erasePartition(const std::string& name) = 0;

    // Reset / lifecycle
    virtual Result<void> uploadLoader(const ByteBuffer& programmerData) = 0;
    virtual Result<void> reset() = 0;
    virtual Result<void> powerReset() = 0;

    // Control
    virtual void cancel() noexcept = 0;
    virtual void setProgressCallback(ProgressCallback callback) = 0;
};

} // namespace mbootcore
