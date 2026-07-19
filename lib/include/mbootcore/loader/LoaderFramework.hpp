#pragma once

#include "mbootcore/domain/Error.hpp"
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/ILogger.hpp"
#include "mbootcore/domain/ILoader.hpp"
#include "mbootcore/loader/ILoaderRepository.hpp"
#include "mbootcore/loader/ILoaderMatcher.hpp"
#include "mbootcore/loader/ILoaderValidator.hpp"
#include "mbootcore/loader/ILoaderCache.hpp"
#include "mbootcore/loader/IELFInspector.hpp"
#include "mbootcore/loader/ISelectionPolicy.hpp"
#include "mbootcore/loader/LoaderMetadata.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <functional>

namespace mbootcore {

class LoaderFramework {
public:
    explicit LoaderFramework(std::shared_ptr<ILogger> logger);

    void setRepository(std::unique_ptr<ILoaderRepository> repo);
    void setMatcher(std::unique_ptr<ILoaderMatcher> matcher);
    void setValidator(std::unique_ptr<ILoaderValidator> validator);
    void setCache(std::unique_ptr<ILoaderCache> cache);
    void setInspector(std::unique_ptr<IElfInspector> inspector);
    void setSelectionPolicy(std::unique_ptr<ISelectionPolicy> policy);

    ILoaderRepository& repository() noexcept { return *m_repo; }
    ILoaderMatcher& matcher() noexcept { return *m_matcher; }
    ILoaderValidator& validator() noexcept { return *m_validator; }
    ILoaderCache& cache() noexcept { return *m_cache; }
    IElfInspector& inspector() noexcept { return *m_inspector; }
    ISelectionPolicy& selectionPolicy() noexcept { return *m_policy; }

    Result<void> addLoader(std::string name, ByteBuffer data, LoaderMetadata metadata);
    Result<std::unique_ptr<ILoaderData>> findBestLoader(const DeviceId& deviceId,
                                                         std::string_view vendor = {},
                                                         std::string_view chipset = {},
                                                         std::string_view protocol = {},
                                                         std::string_view storageType = {});

    Result<std::vector<ILoaderMatcher::MatchResult>> findAll(const DeviceId& deviceId);

    size_t loaderCount() const noexcept;
    std::vector<std::string> listLoaders() const;

    using ProgressCallback = std::function<void(size_t current, size_t total)>;
    void setProgressCallback(ProgressCallback cb);

private:
    std::shared_ptr<ILogger> m_logger;
    std::unique_ptr<ILoaderRepository> m_repo;
    std::unique_ptr<ILoaderMatcher> m_matcher;
    std::unique_ptr<ILoaderValidator> m_validator;
    std::unique_ptr<ILoaderCache> m_cache;
    std::unique_ptr<IElfInspector> m_inspector;
    std::unique_ptr<ISelectionPolicy> m_policy;
    ProgressCallback m_progressCb;

    bool matches(const ILoaderMatcher::MatchResult& result,
                 std::string_view vendor,
                 std::string_view chipset,
                 std::string_view protocol,
                 std::string_view storageType) const noexcept;
};

} // namespace mbootcore
