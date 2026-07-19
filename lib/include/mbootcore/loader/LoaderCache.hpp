#pragma once

#include "mbootcore/loader/ILoaderCache.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace mbootcore {

class LoaderCache : public ILoaderCache {
public:
    explicit LoaderCache(size_t maxEntries = 128);

    bool contains(std::string_view key) const noexcept override;
    Result<std::unique_ptr<ByteBuffer>> get(std::string_view key) override;
    void put(std::string_view key, std::unique_ptr<ByteBuffer> data) override;
    void remove(std::string_view key) noexcept override;
    void clear() noexcept override;
    size_t size() const noexcept override;

    size_t maxEntries() const noexcept { return m_maxEntries; }
    void setMaxEntries(size_t maxEntries) noexcept { m_maxEntries = maxEntries; }

private:
    mutable std::mutex m_mutex;
    size_t m_maxEntries;
    std::map<std::string, std::unique_ptr<ByteBuffer>, std::less<>> m_cache;
    std::vector<std::string> m_accessOrder;

    void evictIfNeeded();
};

} // namespace mbootcore
