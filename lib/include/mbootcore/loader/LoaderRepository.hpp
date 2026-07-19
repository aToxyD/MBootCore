#pragma once

#include "mbootcore/loader/ILoaderRepository.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace mbootcore {

class LoaderRepository : public ILoaderRepository {
public:
    LoaderRepository() = default;

    Result<void> add(std::unique_ptr<ILoaderData> loader) override;
    Result<void> remove(std::string_view name) override;
    Result<std::unique_ptr<ILoaderData>> get(std::string_view name) override;
    std::vector<std::string> list() const override;
    std::vector<std::pair<std::string, LoaderMetadata>> listWithMetadata() const override;
    size_t count() const noexcept override;
    void clear() noexcept override;

    std::unique_ptr<ILoaderData> release(std::string_view name);

private:
    mutable std::mutex m_mutex;
    std::map<std::string, std::unique_ptr<ILoaderData>, std::less<>> m_loaders;
};

} // namespace mbootcore
