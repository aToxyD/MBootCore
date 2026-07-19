#include <mbootcore/security/InMemoryStorage.hpp>
#include <map>
#include <mutex>

namespace mbootcore { namespace security { namespace testing {

struct InMemoryStorage::Impl {
    std::map<std::string, std::string> storage;
    mutable std::mutex mtx;
};

InMemoryStorage::InMemoryStorage()
    : m_impl(std::make_unique<Impl>()) {}

InMemoryStorage::~InMemoryStorage() = default;
InMemoryStorage::InMemoryStorage(InMemoryStorage&&) noexcept = default;
InMemoryStorage& InMemoryStorage::operator=(InMemoryStorage&&) noexcept = default;

Result<void> InMemoryStorage::store(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->storage[key] = value;
    return {};
}

Result<std::string> InMemoryStorage::retrieve(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = m_impl->storage.find(key);
    if (it == m_impl->storage.end()) {
        return ErrorCode::InvalidArgument;
    }
    return it->second;
}

Result<void> InMemoryStorage::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->storage.erase(key);
    return {};
}

Result<bool> InMemoryStorage::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    bool found = (m_impl->storage.find(key) != m_impl->storage.end());
    return found;
}

Result<void> InMemoryStorage::clear() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->storage.clear();
    return {};
}

} } }
