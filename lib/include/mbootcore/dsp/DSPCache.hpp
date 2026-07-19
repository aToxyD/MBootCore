#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

class DSPCache {
public:
    explicit DSPCache(const std::string& cachePath);
    ~DSPCache();

    // Binary metadata cache
    Result<void> cacheMetadata(const std::string& packageId, const DSPPackageMetadata& metadata);
    Result<DSPPackageMetadata> loadMetadata(const std::string& packageId) const;
    bool hasMetadata(const std::string& packageId) const noexcept;
    Result<void> invalidateMetadata(const std::string& packageId);

    // Loader cache
    Result<void> cacheLoader(const std::string& loaderId, const ByteBuffer& data);
    Result<ByteBuffer> loadLoader(const std::string& loaderId) const;
    bool hasLoader(const std::string& loaderId) const noexcept;
    Result<void> invalidateLoader(const std::string& loaderId);

    // Lookup cache
    Result<void> cacheLookup(const std::string& key, const std::string& value);
    Result<std::string> lookupCached(const std::string& key) const;
    bool hasLookup(const std::string& key) const noexcept;

    // Resolved dependency cache
    Result<void> cacheDependencyResolution(const std::string& packageId, const std::vector<std::string>& order);
    Result<std::vector<std::string>> loadDependencyResolution(const std::string& packageId) const;

    // Cache management
    Result<void> clear();
    Result<void> clearAll();
    Result<void> prune(const std::chrono::hours& maxAge = std::chrono::hours{72});
    Result<void> rebuild();
    uint64_t cacheSizeBytes() const noexcept;
    size_t cacheEntryCount() const noexcept;
    bool isWarm() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dsp
} // namespace mbootcore
