#pragma once

#include "mbootcore/loader/ILoaderData.hpp"
#include "mbootcore/loader/LoaderMetadata.hpp"
#include "mbootcore/domain/Error.hpp"

#include <memory>
#include <vector>

namespace mbootcore {

class ILoaderRepository {
public:
    virtual ~ILoaderRepository() = default;

    virtual Result<void> add(std::unique_ptr<ILoaderData> loader) = 0;
    virtual Result<void> remove(std::string_view name) = 0;
    virtual Result<std::unique_ptr<ILoaderData>> get(std::string_view name) = 0;
    virtual std::vector<std::string> list() const = 0;
    virtual std::vector<std::pair<std::string, LoaderMetadata>> listWithMetadata() const = 0;
    virtual size_t count() const noexcept = 0;
    virtual void clear() noexcept = 0;
};

} // namespace mbootcore
