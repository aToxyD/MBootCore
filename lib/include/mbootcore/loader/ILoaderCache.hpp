#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"

#include <string>
#include <memory>

namespace mbootcore {

class ILoaderCache {
public:
    virtual ~ILoaderCache() = default;

    virtual bool contains(std::string_view key) const noexcept = 0;
    virtual Result<std::unique_ptr<ByteBuffer>> get(std::string_view key) = 0;
    virtual void put(std::string_view key, std::unique_ptr<ByteBuffer> data) = 0;
    virtual void remove(std::string_view key) noexcept = 0;
    virtual void clear() noexcept = 0;
    virtual size_t size() const noexcept = 0;
};

} // namespace mbootcore
