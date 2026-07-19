#include "mbootcore/loader/LoaderMatcher.hpp"
#include "mbootcore/loader/LoaderRepository.hpp"
#include "mbootcore/loader/PrioritySelection.hpp"

#include <algorithm>
#include <cstring>

namespace mbootcore {

LoaderMatcher::LoaderMatcher(std::shared_ptr<ISelectionPolicy> policy)
    : m_policy(policy ? std::move(policy) : std::make_shared<PrioritySelection>()) {}

void LoaderMatcher::setDeviceIds(const DeviceId& deviceId) {
    m_criteria.deviceId = deviceId;
}

void LoaderMatcher::setVendor(std::string_view vendor) {
    m_criteria.vendor = vendor;
}

void LoaderMatcher::setChipset(std::string_view chipset) {
    m_criteria.chipset = chipset;
}

void LoaderMatcher::setStorageType(std::string_view storageType) {
    m_criteria.storageType = storageType;
}

void LoaderMatcher::setProtocol(std::string_view protocol) {
    m_criteria.protocol = protocol;
}

uint32_t LoaderMatcher::computeScore(const LoaderMetadata& meta, const MatchCriteria& criteria) {
    uint32_t score = 0;

    if (!criteria.vendor.empty() && meta.vendor == criteria.vendor) {
        score += 40;
    }

    if (!criteria.chipset.empty() && meta.chipset == criteria.chipset) {
        score += 30;
    }

    if (!criteria.protocol.empty() && meta.protocol == criteria.protocol) {
        score += 15;
    }

    if (!criteria.storageType.empty() && meta.storageType == criteria.storageType) {
        score += 10;
    }

    if (!meta.pkhash.empty() && !isPkHashEmpty(criteria.deviceId.pkhash) &&
        meta.pkhash.size() == 32) {
        bool match = true;
        for (size_t i = 0; i < 32; ++i) {
            if (meta.pkhash[i] != criteria.deviceId.pkhash[i]) {
                match = false;
                break;
            }
        }
        if (match) score += 80;
    }

    if (criteria.deviceId.msmId > 0 && meta.msmId == criteria.deviceId.msmId) {
        score += 50;
    }

    return score;
}

bool LoaderMatcher::matchVendor(const LoaderMetadata& meta) const noexcept {
    return m_criteria.vendor.empty() || meta.vendor == m_criteria.vendor;
}

bool LoaderMatcher::matchProtocol(const LoaderMetadata& meta) const noexcept {
    return m_criteria.protocol.empty() || meta.protocol == m_criteria.protocol;
}

bool LoaderMatcher::matchChipset(const LoaderMetadata& meta) const noexcept {
    return m_criteria.chipset.empty() || meta.chipset == m_criteria.chipset;
}

bool LoaderMatcher::matchMsmId(const LoaderMetadata& meta) const noexcept {
    if (m_criteria.deviceId.msmId == 0) return true;
    return meta.msmId == m_criteria.deviceId.msmId;
}

bool LoaderMatcher::isPkHashEmpty(const std::array<uint8_t, 32>& arr) noexcept {
    for (auto b : arr) {
        if (b != 0) return false;
    }
    return true;
}

bool LoaderMatcher::matchPkHash(const LoaderMetadata& meta) const noexcept {
    if (isPkHashEmpty(m_criteria.deviceId.pkhash)) return true;
    if (meta.pkhash.empty()) return true;
    if (meta.pkhash.size() != 32) return false;
    return std::memcmp(meta.pkhash.data(), m_criteria.deviceId.pkhash.data(), 32) == 0;
}

bool LoaderMatcher::matchStorageType(const LoaderMetadata& meta) const noexcept {
    return m_criteria.storageType.empty() || meta.storageType == m_criteria.storageType;
}

std::vector<ILoaderMatcher::MatchResult> LoaderMatcher::find(
    const std::vector<std::pair<std::string, LoaderMetadata>>& candidates)
{
    std::vector<MatchResult> results;
    results.reserve(candidates.size());

    for (const auto& [name, meta] : candidates) {
        if (!matchVendor(meta)) continue;
        if (!matchProtocol(meta)) continue;
        if (!matchMsmId(meta)) continue;
        if (!matchStorageType(meta)) continue;

        MatchResult result;
        result.name = name;
        result.metadata = meta;
        result.score = computeScore(meta, m_criteria);
        results.push_back(std::move(result));
    }

    std::stable_sort(results.begin(), results.end(),
        [](const MatchResult& a, const MatchResult& b) {
            return a.score > b.score;
        });

    return results;
}

} // namespace mbootcore
