#include "mbootcore/loader/LoaderCache.hpp"

#include <algorithm>

namespace mbootcore {

LoaderCache::LoaderCache(size_t maxEntries)
    : m_maxEntries(maxEntries) {}

bool LoaderCache::contains(std::string_view key) const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.find(key) != m_cache.end();
}

Result<std::unique_ptr<ByteBuffer>> LoaderCache::get(std::string_view key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return ErrorCode::CacheMiss;
    }
    auto copy = std::make_unique<ByteBuffer>(*it->second);
    return copy;
}

void LoaderCache::put(std::string_view key, std::unique_ptr<ByteBuffer> data) {
    if (!data) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto k = std::string(key);
    m_cache[k] = std::move(data);
    m_accessOrder.push_back(k);
    evictIfNeeded();
}

void LoaderCache::remove(std::string_view key) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto k = std::string(key);
    m_cache.erase(k);
    m_accessOrder.erase(
        std::remove(m_accessOrder.begin(), m_accessOrder.end(), k),
        m_accessOrder.end());
}

void LoaderCache::clear() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_accessOrder.clear();
}

size_t LoaderCache::size() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.size();
}

void LoaderCache::evictIfNeeded() {
    while (m_cache.size() > m_maxEntries && !m_accessOrder.empty()) {
        auto oldest = m_accessOrder.front();
        m_accessOrder.erase(m_accessOrder.begin());
        m_cache.erase(oldest);
    }
}

} // namespace mbootcore
