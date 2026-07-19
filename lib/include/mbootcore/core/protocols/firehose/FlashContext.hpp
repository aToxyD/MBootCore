#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/generic/StorageInfo.hpp"

#include <string>
#include <chrono>
#include <cstdint>

namespace mbootcore {

struct FlashContext {
    std::string memoryName{"ufs"};
    StorageType storageType{StorageType::UFS};
    uint32_t lun{0};
    uint32_t sectorSize{4096};
    uint64_t totalSectors{0};
    uint64_t capacityBytes{0};
    uint32_t maxPayloadToTarget{1048576};
    uint32_t maxPayloadFromTarget{1048576};

    bool eraseSupported{true};
    bool programSupported{true};
    bool readSupported{true};
    bool verifySupported{true};

    std::chrono::milliseconds commandTimeout{5000};
    std::chrono::milliseconds chunkTimeout{10000};
    int maxRetries{3};

    uint32_t numPartitions{0};
    std::vector<std::string> partitionNames;
};

} // namespace mbootcore
