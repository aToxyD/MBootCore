#pragma once

#include "mbootcore/loader/ILoaderMatcher.hpp"
#include "mbootcore/loader/ILoaderData.hpp"
#include "mbootcore/loader/ISelectionPolicy.hpp"

#include <functional>
#include <vector>

namespace mbootcore {

class LoaderMatcher : public ILoaderMatcher {
public:
    explicit LoaderMatcher(std::shared_ptr<ISelectionPolicy> policy = nullptr);

    void setDeviceIds(const DeviceId& deviceId) override;
    void setVendor(std::string_view vendor) override;
    void setChipset(std::string_view chipset) override;
    void setStorageType(std::string_view storageType) override;
    void setProtocol(std::string_view protocol) override;

    std::vector<MatchResult> find(const std::vector<std::pair<std::string, LoaderMetadata>>& candidates) override;

    struct MatchCriteria {
        std::string vendor;
        std::string chipset;
        std::string protocol;
        std::string storageType;
        DeviceId deviceId;
    };

    const MatchCriteria& criteria() const noexcept { return m_criteria; }

    static uint32_t computeScore(const LoaderMetadata& meta, const MatchCriteria& criteria);

private:
    std::shared_ptr<ISelectionPolicy> m_policy;
    MatchCriteria m_criteria;

    bool matchVendor(const LoaderMetadata& meta) const noexcept;
    bool matchProtocol(const LoaderMetadata& meta) const noexcept;
    bool matchChipset(const LoaderMetadata& meta) const noexcept;
    bool matchMsmId(const LoaderMetadata& meta) const noexcept;
    bool matchPkHash(const LoaderMetadata& meta) const noexcept;
    static bool isPkHashEmpty(const std::array<uint8_t, 32>& arr) noexcept;
    bool matchStorageType(const LoaderMetadata& meta) const noexcept;
};

} // namespace mbootcore
