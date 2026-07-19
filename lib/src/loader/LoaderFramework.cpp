#include "mbootcore/loader/LoaderFramework.hpp"
#include "mbootcore/loader/LoaderRepository.hpp"
#include "mbootcore/loader/LoaderMatcher.hpp"
#include "mbootcore/loader/LoaderValidator.hpp"
#include "mbootcore/loader/LoaderCache.hpp"
#include "mbootcore/loader/ElfInspector.hpp"
#include "mbootcore/loader/PrioritySelection.hpp"
#include "mbootcore/loader/LoaderData.hpp"

namespace mbootcore {

LoaderFramework::LoaderFramework(std::shared_ptr<ILogger> logger)
    : m_logger(std::move(logger))
    , m_repo(std::make_unique<LoaderRepository>())
    , m_matcher(std::make_unique<LoaderMatcher>())
    , m_validator(std::make_unique<LoaderValidator>())
    , m_cache(std::make_unique<LoaderCache>())
    , m_inspector(std::make_unique<ElfInspector>())
    , m_policy(std::make_unique<PrioritySelection>()) {}

void LoaderFramework::setRepository(std::unique_ptr<ILoaderRepository> repo) {
    m_repo = std::move(repo);
}

void LoaderFramework::setMatcher(std::unique_ptr<ILoaderMatcher> matcher) {
    m_matcher = std::move(matcher);
}

void LoaderFramework::setValidator(std::unique_ptr<ILoaderValidator> validator) {
    m_validator = std::move(validator);
}

void LoaderFramework::setCache(std::unique_ptr<ILoaderCache> cache) {
    m_cache = std::move(cache);
}

void LoaderFramework::setInspector(std::unique_ptr<IElfInspector> inspector) {
    m_inspector = std::move(inspector);
}

void LoaderFramework::setSelectionPolicy(std::unique_ptr<ISelectionPolicy> policy) {
    m_policy = std::move(policy);
}

Result<void> LoaderFramework::addLoader(std::string name, ByteBuffer data, LoaderMetadata metadata) {
    if (name.empty()) {
        return ErrorCode::InvalidArgument;
    }

    metadata.loaderSize = data.size();
    auto loader = std::make_unique<LoaderData>(std::move(name), std::move(data), std::move(metadata));
    return m_repo->add(std::move(loader));
}

bool LoaderFramework::matches(const ILoaderMatcher::MatchResult& result,
                              std::string_view vendor,
                              std::string_view chipset,
                              std::string_view protocol,
                              std::string_view storageType) const noexcept {
    if (!vendor.empty() && result.metadata.vendor != vendor) return false;
    if (!chipset.empty() && result.metadata.chipset != chipset) return false;
    if (!protocol.empty() && result.metadata.protocol != protocol) return false;
    if (!storageType.empty() && result.metadata.storageType != storageType) return false;
    return true;
}

Result<std::unique_ptr<ILoaderData>> LoaderFramework::findBestLoader(
    const DeviceId& deviceId,
    std::string_view vendor,
    std::string_view chipset,
    std::string_view protocol,
    std::string_view storageType)
{
    auto all = findAll(deviceId);
    if (all.isError()) {
        return all.error();
    }

    auto& results = all.value();
    results.erase(std::remove_if(results.begin(), results.end(),
        [&](const ILoaderMatcher::MatchResult& r) {
            return !matches(r, vendor, chipset, protocol, storageType);
        }), results.end());

    if (results.empty()) {
        return ErrorCode::LoaderNotFound;
    }

    std::vector<ISelectionPolicy::Candidate> candidates;
    candidates.reserve(results.size());
    for (const auto& result : results) {
        ISelectionPolicy::Candidate cand;
        cand.name = result.name;
        cand.metadata = result.metadata;
        cand.priority = static_cast<SelectionPriority>(result.score);
        candidates.push_back(std::move(cand));
    }

    auto selected = m_policy->select(candidates);
    if (selected.empty()) {
        return ErrorCode::LoaderNotFound;
    }

    return m_repo->get(selected);
}

Result<std::vector<ILoaderMatcher::MatchResult>> LoaderFramework::findAll(
    const DeviceId& deviceId)
{
    auto candidates = m_repo->listWithMetadata();
    if (candidates.empty()) {
        return ErrorCode::LoaderNotFound;
    }

    m_matcher->setDeviceIds(deviceId);
    auto results = m_matcher->find(candidates);

    if (results.empty()) {
        return ErrorCode::LoaderNotFound;
    }

    return results;
}

size_t LoaderFramework::loaderCount() const noexcept {
    return m_repo->count();
}

std::vector<std::string> LoaderFramework::listLoaders() const {
    return m_repo->list();
}

void LoaderFramework::setProgressCallback(ProgressCallback cb) {
    m_progressCb = std::move(cb);
}

} // namespace mbootcore
