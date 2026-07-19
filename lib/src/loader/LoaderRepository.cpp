#include "mbootcore/loader/LoaderRepository.hpp"
#include "mbootcore/loader/LoaderData.hpp"

namespace mbootcore {

Result<void> LoaderRepository::add(std::unique_ptr<ILoaderData> loader) {
    if (!loader) {
        return ErrorCode::InvalidArgument;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto key = std::string(loader->name());
    if (m_loaders.find(key) != m_loaders.end()) {
        return ErrorCode::AlreadyExists;
    }
    m_loaders.emplace(std::move(key), std::move(loader));
    return {};
}

Result<void> LoaderRepository::remove(std::string_view name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loaders.find(name);
    if (it == m_loaders.end()) {
        return ErrorCode::LoaderNotFound;
    }
    m_loaders.erase(it);
    return {};
}

Result<std::unique_ptr<ILoaderData>> LoaderRepository::get(std::string_view name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loaders.find(name);
    if (it == m_loaders.end()) {
        return ErrorCode::LoaderNotFound;
    }
    auto copy = std::make_unique<LoaderData>(
        std::string(it->second->name()),
        ByteBuffer(it->second->data()),
        it->second->metadata());
    return Result<std::unique_ptr<ILoaderData>>(std::move(copy));
}

std::vector<std::string> LoaderRepository::list() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    names.reserve(m_loaders.size());
    for (const auto& [name, _] : m_loaders) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::pair<std::string, LoaderMetadata>> LoaderRepository::listWithMetadata() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::pair<std::string, LoaderMetadata>> result;
    result.reserve(m_loaders.size());
    for (const auto& [name, loader] : m_loaders) {
        result.emplace_back(name, loader->metadata());
    }
    return result;
}

size_t LoaderRepository::count() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loaders.size();
}

void LoaderRepository::clear() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loaders.clear();
}

std::unique_ptr<ILoaderData> LoaderRepository::release(std::string_view name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loaders.find(name);
    if (it == m_loaders.end()) return nullptr;
    auto ptr = std::move(it->second);
    m_loaders.erase(it);
    return ptr;
}

} // namespace mbootcore
