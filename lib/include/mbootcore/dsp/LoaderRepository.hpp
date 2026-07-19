#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

class DSPRepository;

class LoaderIndex {
public:
    explicit LoaderIndex(const DSPRepository& repo);

    void refresh();
    size_t count() const noexcept;
    std::vector<std::string> allLoaderIds() const;
    const DSPLoaderMetadata* byId(const std::string& loaderId) const;
    std::vector<const DSPLoaderMetadata*> byChipset(const ChipsetId& id) const;
    std::vector<const DSPLoaderMetadata*> byProtocol(discovery::ProtocolType proto) const;
    std::vector<const DSPLoaderMetadata*> byVendor(discovery::Vendor vendor) const;
    std::vector<const DSPLoaderMetadata*> search(const std::string& query) const;

private:
    const DSPRepository& m_repo;
    std::vector<const DSPLoaderMetadata*> m_index;
};

class LoaderDatabase {
public:
    explicit LoaderDatabase(const DSPRepository& repo);

    Result<void> rebuild();
    const DSPLoaderMetadata* findById(const std::string& loaderId) const;
    std::vector<const DSPLoaderMetadata*> findByChipset(const ChipsetId& id) const;
    std::vector<const DSPLoaderMetadata*> findByProtocol(discovery::ProtocolType proto) const;
    std::vector<const DSPLoaderMetadata*> findByVendor(discovery::Vendor vendor) const;
    std::vector<const DSPLoaderMetadata*> findByBootMode(discovery::BootMode mode) const;
    std::vector<const DSPLoaderMetadata*> findWithTag(const std::string& tag) const;
    size_t count() const noexcept;

private:
    const DSPRepository& m_repo;
    std::vector<const DSPLoaderMetadata*> m_entries;
};

class LoaderMatcher {
public:
    struct MatchResult {
        const DSPLoaderMetadata* loader{nullptr};
        int score{0};
        std::string reason;
    };

    explicit LoaderMatcher(const LoaderDatabase& db);

    std::vector<MatchResult> findBest(const ChipsetId& chipset,
        discovery::ProtocolType protocol = discovery::ProtocolType::Unknown,
        discovery::BootMode mode = discovery::BootMode::Unknown) const;
    const DSPLoaderMetadata* findBestSingle(const ChipsetId& chipset,
        discovery::ProtocolType protocol = discovery::ProtocolType::Unknown,
        discovery::BootMode mode = discovery::BootMode::Unknown) const;

private:
    const LoaderDatabase& m_db;
    int scoreLoader(const DSPLoaderMetadata& loader, const ChipsetId& chipset,
        discovery::ProtocolType protocol, discovery::BootMode mode) const;
};

class LoaderResolver {
public:
    explicit LoaderResolver(const LoaderDatabase& db, const LoaderMatcher& matcher);

    struct ResolvedLoader {
        const DSPLoaderMetadata* primary{nullptr};
        std::vector<const DSPLoaderMetadata*> fallbacks;
    };

    ResolvedLoader resolve(const ChipsetId& chipset,
        discovery::ProtocolType protocol = discovery::ProtocolType::Unknown,
        discovery::BootMode mode = discovery::BootMode::Unknown) const;

private:
    const LoaderDatabase& m_db;
    const LoaderMatcher& m_matcher;
};

class LoaderCache {
public:
    explicit LoaderCache(const std::string& cachePath);
    ~LoaderCache();

    Result<void> cacheLoader(const std::string& loaderId, const ByteBuffer& data);
    Result<ByteBuffer> loadCached(const std::string& loaderId) const;
    bool isCached(const std::string& loaderId) const noexcept;
    Result<void> invalidate(const std::string& loaderId);
    Result<void> clear();
    size_t cachedCount() const noexcept;
    uint64_t totalCachedSize() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class LoaderVerifier {
public:
    Result<bool> verifyIntegrity(const DSPLoaderMetadata& loader, const ByteBuffer& data) const;
    Result<bool> verifyHash(const DSPLoaderMetadata& loader, const ByteBuffer& data) const;
    Result<bool> verifySignature(const DSPLoaderMetadata& loader, const ByteBuffer& data) const;
    Result<bool> verifyCompatibility(const DSPLoaderMetadata& loader) const;
};

} // namespace dsp
} // namespace mbootcore
