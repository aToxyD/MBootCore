#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>

namespace mbootcore {
namespace dsp {

struct StorageProfile {
    StorageType type{StorageType::eMMC};
    uint64_t totalSizeMB{0};
    uint32_t sectorSize{512};
    uint32_t blockSize{4096};
    uint32_t eraseBlockSize{4194304};
    uint64_t maxPartitionSize{0};
    uint32_t maxPartitionCount{32};
    bool supportsTRIM{false};
    bool supportsSanitize{false};
    bool supportsHardReset{false};
    bool supportsCache{false};
    uint32_t maxWriteSpeedMBps{0};
    uint32_t maxReadSpeedMBps{0};
};

struct MemoryProfile {
    uint64_t totalMB{0};
    uint64_t availableMB{0};
    uint64_t reservedMB{0};
    uint64_t ddrFreqMHz{0};
    uint32_t channelCount{1};
    bool eccSupported{false};
    bool sharedWithGPU{false};
};

struct PartitionLayout {
    std::string name;
    uint64_t startSector{0};
    uint64_t endSector{0};
    uint64_t sizeSectors{0};
    std::string guid;
    std::string typeGuid;
    std::string partitionType;
    std::string filesystem;
    bool bootable{false};
    bool readonly{false};
    bool hidden{false};
    bool required{true};
    std::vector<std::string> attributes;
};

struct FlashGeometry {
    StorageType storageType{StorageType::eMMC};
    uint64_t totalSectors{0};
    uint32_t sectorSize{512};
    uint64_t totalSizeBytes{0};
    uint32_t logicalBlockSize{512};
    uint32_t physicalBlockSize{4096};
    uint32_t eraseGroupSize{4194304};
    uint64_t availableSectors{0};
    uint64_t maxProgramSize{0};
    uint32_t maxProgramChunks{32};
    bool supportsSparse{false};
    bool writeProtectSupported{false};
};

struct CapabilityProfile {
    bool canProgram{false};
    bool canErase{false};
    bool canRead{false};
    bool canVerify{false};
    bool canBackup{false};
    bool canRestore{false};
    bool canCompare{false};
    bool canResume{false};
    bool canRollback{false};
    bool canHotplug{false};
    bool canAsync{false};
    bool canMultiSession{false};
    bool canSparse{false};
    bool canCompress{false};
    bool canEncrypt{false};
    bool canSign{false};
    bool canAuth{false};
};

struct SecurityProfile {
    bool secureBoot{false};
    bool trustZone{false};
    bool hardwareCrypto{false};
    bool debugPort{false};
    bool signedImages{false};
    bool encryptedStorage{false};
    bool authRequired{false};
    bool rpmb{false};
    uint32_t maxAuthRetries{3};
    std::vector<std::string> securityFeatures;
};

struct PerformanceProfile {
    uint32_t recommendedWorkers{4};
    uint32_t maxWorkers{8};
    uint32_t bufferSizeKB{1024};
    uint32_t maxTransferSizeMB{16};
    uint32_t recommendedTimeoutMs{30000};
    bool useAsyncIO{true};
    bool useBulkTransfer{true};
    bool useStreamMode{true};
    uint32_t maxRetries{3};
    std::chrono::milliseconds retryDelay{1000};
};

struct HardwareProfile {
    std::string profileId;
    std::string name;
    std::string description;
    ChipsetId chipset;
    StorageProfile storage;
    MemoryProfile memory;
    std::vector<PartitionLayout> partitionLayout;
    FlashGeometry flashGeometry;
    CapabilityProfile capabilities;
    SecurityProfile security;
    PerformanceProfile performance;
    std::unordered_map<std::string, std::string> customProperties;
    DSPVersion version;
    bool isDefault{false};
};

class HardwareProfileManager {
public:
    explicit HardwareProfileManager(const DSPRepository& repo);
    ~HardwareProfileManager();

    Result<void> rebuild();

    const HardwareProfile* findById(const std::string& profileId) const;
    std::vector<const HardwareProfile*> findByChipset(const ChipsetId& id) const;
    const HardwareProfile* findDefault(const ChipsetId& id) const;
    std::vector<const HardwareProfile*> search(const std::string& query) const;
    size_t count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dsp
} // namespace mbootcore
