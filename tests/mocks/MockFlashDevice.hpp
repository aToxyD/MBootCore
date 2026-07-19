#pragma once

#include "mbootcore/generic/IFlashDevice.hpp"
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <mutex>

namespace mbootcore {
namespace gpt {

class MockFlashDevice : public IFlashDevice {
public:
    explicit MockFlashDevice(uint64_t numSectors = 1024, uint32_t sectorSize = 512)
        : m_storage(numSectors * sectorSize, 0)
        , m_numSectors(numSectors)
        , m_sectorSize(sectorSize) {}

    void initGPT(uint64_t primaryLBA, uint64_t backupLBA,
                 const std::vector<uint8_t>& primaryData,
                 const std::vector<uint8_t>& backupData);

    ByteBuffer& rawStorage() { return m_storage; }
    const ByteBuffer& rawStorage() const { return m_storage; }

    void injectError(uint64_t offset, uint8_t value) {
        if (offset < m_storage.size()) m_storage[offset] = value;
    }

    void setCorrupt(bool corrupt) { m_corrupt = corrupt; }

    // IFlashDevice
    Result<void> open() override { m_open = true; return Result<void>::Ok(); }
    void close() noexcept override { m_open = false; }
    bool isOpen() const noexcept override { return m_open; }
    FlashCapability capabilities() const noexcept override {
        return FlashCapability::Read | FlashCapability::Write | FlashCapability::Erase;
    }
    Result<GenericDeviceInfo> deviceInfo() override {
        return Result<GenericDeviceInfo>::Ok(GenericDeviceInfo{});
    }
    Result<StorageInfo> getStorageInfo() override {
        StorageInfo info;
        info.type = StorageType::eMMC;
        info.name = "mock_device";
        info.numSectors = m_numSectors;
        info.sectorSize = m_sectorSize;
        info.capacity = m_numSectors * m_sectorSize;
        return Result<StorageInfo>::Ok(info);
    }
    Result<PartitionTable> getPartitions() override {
        return Result<PartitionTable>::Error(ErrorCode::NotSupported);
    }

    Result<ByteBuffer> readMemory(uint64_t address, size_t size) override {
        if (m_corrupt) return Result<ByteBuffer>::Error(ErrorCode::TransportError);
        if (address + size > m_storage.size())
            return Result<ByteBuffer>::Error(ErrorCode::InvalidArgument);
        ByteBuffer buf(m_storage.begin() + static_cast<ptrdiff_t>(address),
                       m_storage.begin() + static_cast<ptrdiff_t>(address + size));
        return Result<ByteBuffer>::Ok(std::move(buf));
    }

    Result<void> writeMemory(uint64_t address, const ByteBuffer& data) override {
        if (m_corrupt) return Result<void>::Error(ErrorCode::TransportError);
        if (address + data.size() > m_storage.size())
            return Result<void>::Error(ErrorCode::InvalidArgument);
        std::copy(data.begin(), data.end(),
                  m_storage.begin() + static_cast<ptrdiff_t>(address));
        return Result<void>::Ok();
    }

    Result<void> eraseMemory(uint64_t address, size_t size) override {
        if (m_corrupt) return Result<void>::Error(ErrorCode::TransportError);
        if (address + size > m_storage.size())
            return Result<void>::Error(ErrorCode::InvalidArgument);
        std::fill(m_storage.begin() + static_cast<ptrdiff_t>(address),
                  m_storage.begin() + static_cast<ptrdiff_t>(address + size), 0);
        return Result<void>::Ok();
    }

    Result<ByteBuffer> readPartition(const std::string&) override {
        return Result<ByteBuffer>::Error(ErrorCode::NotSupported);
    }
    Result<void> writePartition(const std::string&, const ByteBuffer&) override {
        return Result<void>::Error(ErrorCode::NotSupported);
    }
    Result<void> erasePartition(const std::string&) override {
        return Result<void>::Error(ErrorCode::NotSupported);
    }
    Result<void> uploadLoader(const ByteBuffer&) override {
        return Result<void>::Error(ErrorCode::NotSupported);
    }
    Result<void> reset() override {
        return Result<void>::Ok();
    }
    Result<void> powerReset() override {
        return Result<void>::Ok();
    }
    void cancel() noexcept override {}
    void setProgressCallback(ProgressCallback) override {}

private:
    ByteBuffer m_storage;
    uint64_t m_numSectors;
    uint32_t m_sectorSize;
    bool m_open{false};
    bool m_corrupt{false};
};

}} // namespace mbootcore::gpt
