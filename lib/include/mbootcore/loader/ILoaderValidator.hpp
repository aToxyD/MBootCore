#pragma once

#include "mbootcore/domain/Error.hpp"
#include "mbootcore/loader/LoaderMetadata.hpp"

#include <string_view>

namespace mbootcore {

class ILoaderValidator {
public:
    virtual ~ILoaderValidator() = default;

    virtual bool hasRequiredMetadata(const LoaderMetadata& meta) const noexcept = 0;
    virtual bool isHashValid(const LoaderMetadata& meta) const noexcept = 0;
    virtual bool isCompatible(const LoaderMetadata& meta) const noexcept = 0;
    virtual bool isCorrupted(const LoaderMetadata& meta) const noexcept = 0;
    virtual std::vector<std::string> validationErrors(const LoaderMetadata& meta) const = 0;
};

} // namespace mbootcore
