#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/loader/LoaderMetadata.hpp"

#include <memory>
#include <vector>

namespace mbootcore {

class ILoaderMatcher {
public:
    virtual ~ILoaderMatcher() = default;

    struct MatchResult {
        std::string name;
        LoaderMetadata metadata;
        uint32_t score{0};
    };

    virtual std::vector<MatchResult> find(const std::vector<std::pair<std::string, LoaderMetadata>>& candidates) = 0;
    virtual void setDeviceIds(const DeviceId& deviceId) = 0;
    virtual void setVendor(std::string_view vendor) = 0;
    virtual void setChipset(std::string_view chipset) = 0;
    virtual void setStorageType(std::string_view storageType) = 0;
    virtual void setProtocol(std::string_view protocol) = 0;
};

} // namespace mbootcore
