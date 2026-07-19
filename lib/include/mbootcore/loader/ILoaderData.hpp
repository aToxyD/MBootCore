#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/loader/LoaderMetadata.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace mbootcore {

class ILoaderData {
public:
    virtual ~ILoaderData() = default;

    virtual const ByteBuffer& data() const noexcept = 0;
    virtual const LoaderMetadata& metadata() const noexcept = 0;
    virtual std::string_view name() const noexcept = 0;
};

} // namespace mbootcore
