#pragma once

#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>
#include <mbootcore/domain/Error.hpp>

#include <vector>
#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <set>
#include <random>
#include <atomic>
#include <chrono>
#include <thread>

namespace mbootcore {
namespace job {

struct FailureRule {
    bool failOpen{false};
    bool failClose{false};
    bool failRead{false};
    bool failWrite{false};
    bool failErase{false};
    bool failUpload{false};
    bool failReset{false};
    bool failGetInfo{false};
    bool failGetPartitions{false};

    bool timeoutOnRead{false};
    bool timeoutOnWrite{false};
    bool timeoutOnErase{false};

    bool disconnectOnRead{false};
    bool disconnectOnWrite{false};
    bool disconnectOnErase{false};

    double randomFailureRate{0.0};
    int seed{42};

    bool badGPT{false};
    bool corruptData{false};
    bool rollbackSuccess{true};
    bool rollbackFailure{false};
};

class VirtualJobDevice : public IFlashDevice {
public:
    VirtualJobDevice();
    explicit VirtualJobDevice(uint64_t storageSize);

    void setFailureRules(const FailureRule& rules) { m_rules = rules; }
    FailureRule& failureRules() noexcept { return m_rules; }
    const FailureRule& failureRules() const noexcept { return m_rules; }

    ByteBuffer& storage() noexcept { return m_storage; }
    const ByteBuffer& storage() const noexcept { return m_storage; }

    void setStorageSize(uint64_t bytes);
    void clearStorage();
    void fillStorage(uint8_t value);
    void writeToStorage(uint64_t offset, const ByteBuffer& data);
    ByteBuffer readFromStorage(uint64_t offset, size_t size) const;

    void setDeviceInfo(const GenericDeviceInfo& info) { m_deviceInfo = info; }
    void setCapabilities(FlashCapability caps) { m_caps = caps; }

    void setTimeouts(std::chrono::milliseconds readDelay,
                     std::chrono::milliseconds writeDelay,
                     std::chrono::milliseconds eraseDelay);

    int openCount() const noexcept { return m_openCount; }
    int closeCount() const noexcept { return m_closeCount; }
    int readCount() const noexcept { return m_readCount; }
    int writeCount() const noexcept { return m_writeCount; }
    int eraseCount() const noexcept { return m_eraseCount; }

    void resetCounters();

    // Partition simulation
    struct PartInfo {
        std::string name;
        uint64_t offset;
        uint64_t size;
    };

    void addPartition(const std::string& name, uint64_t offset, uint64_t size);
    void clearPartitions();

    // IFlashDevice
    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override { return m_open; }
    FlashCapability capabilities() const noexcept override { return m_caps; }
    Result<GenericDeviceInfo> deviceInfo() override;
    Result<StorageInfo> getStorageInfo() override;
    Result<PartitionTable> getPartitions() override;
    Result<ByteBuffer> readMemory(uint64_t address, size_t size) override;
    Result<void> writeMemory(uint64_t address, const ByteBuffer& data) override;
    Result<void> eraseMemory(uint64_t address, size_t size) override;
    Result<ByteBuffer> readPartition(const std::string& name) override;
    Result<void> writePartition(const std::string& name, const ByteBuffer& data) override;
    Result<void> erasePartition(const std::string& name) override;
    Result<void> uploadLoader(const ByteBuffer& data) override;
    Result<void> reset() override;
    Result<void> powerReset() override;
    void cancel() noexcept override { m_cancelled = true; }
    void setProgressCallback(ProgressCallback callback) override { m_progressCb = std::move(callback); }

private:
    bool shouldRandomFail() const;
    void simulateDelay(std::chrono::milliseconds delay) const;
    void reportProgress(uint64_t current, uint64_t total,
                        const std::string& op, const std::string& stage);
    Result<PartInfo> findPartition(const std::string& name) const;

    ByteBuffer m_storage;
    uint64_t m_storageSize{0};
    FailureRule m_rules;
    GenericDeviceInfo m_deviceInfo;
    FlashCapability m_caps{FlashCapability::All};
    std::vector<PartInfo> m_partitions;

    bool m_open{false};
    std::atomic<bool> m_cancelled{false};
    ProgressCallback m_progressCb;

    std::chrono::milliseconds m_readDelay{0};
    std::chrono::milliseconds m_writeDelay{0};
    std::chrono::milliseconds m_eraseDelay{0};

    int m_openCount{0};
    int m_closeCount{0};
    int m_readCount{0};
    int m_writeCount{0};
    int m_eraseCount{0};

    mutable std::mt19937 m_rng;
};

} // namespace job
} // namespace mbootcore
