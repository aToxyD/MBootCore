#pragma once

#include <mbootcore/domain/Types.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/IProtocol.hpp>
#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/domain/ILoader.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/loader/LoaderMetadata.hpp>
#include <mbootcore/elf/ElfModels.hpp>
#include <mbootcore/generic/DeviceInfo.hpp>
#include <mbootcore/generic/StorageInfo.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>
#include <mbootcore/generic/FlashCapability.hpp>
#include <mbootcore/pipeline/PipelineStage.hpp>

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace mbootcore {
namespace pipeline {

struct BootContext {
    DeviceId deviceId;
    DeviceInfo saharaDeviceInfo;
    GenericDeviceInfo genericDeviceInfo;
    ProtocolVersion negotiatedVersion;

    LoaderMetadata loaderMetadata;

    elf::MemoryImage memoryImage;

    struct {
        std::string memoryName{"ufs"};
        StorageType storageType{StorageType::UFS};
        uint32_t lun{0};
        uint64_t sectorSize{4096};
        uint64_t totalSectors{0};
        uint64_t capacityBytes{0};
        uint64_t maxPayloadToTarget{1048576};
        uint64_t maxPayloadFromTarget{1048576};
        bool eraseSupported{true};
        bool programSupported{true};
        bool readSupported{true};
        bool verifySupported{true};
        int commandTimeoutMs{5000};
        int chunkTimeoutMs{10000};
        int maxRetries{3};
        std::vector<std::string> partitionNames;
    } flashConfig;

    ProgressInfo progress;

    PipelineStage currentStage{PipelineStage::Disconnected};
    PipelineStage previousStage{PipelineStage::Disconnected};
    ErrorCode lastError{ErrorCode::Success};

    int stageRetryCount{0};

    std::unordered_map<std::string, std::string> properties;

    ITransport* transport{nullptr};
    ILogger* logger{nullptr};

    BootContext() = default;
    BootContext(const BootContext&) = delete;
    BootContext& operator=(const BootContext&) = delete;
    BootContext(BootContext&&) = default;
    BootContext& operator=(BootContext&&) = default;
};

} // namespace pipeline
} // namespace mbootcore
