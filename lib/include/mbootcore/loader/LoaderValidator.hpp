#pragma once

#include "mbootcore/loader/ILoaderValidator.hpp"

namespace mbootcore {

class LoaderValidator : public ILoaderValidator {
public:
    explicit LoaderValidator(const LoaderMetadata& reference = {});

    void setReference(const LoaderMetadata& ref) noexcept { m_reference = ref; }

    bool hasRequiredMetadata(const LoaderMetadata& meta) const noexcept override;
    bool isHashValid(const LoaderMetadata& meta) const noexcept override;
    bool isCompatible(const LoaderMetadata& meta) const noexcept override;
    bool isCorrupted(const LoaderMetadata& meta) const noexcept override;
    std::vector<std::string> validationErrors(const LoaderMetadata& meta) const override;

    static bool validateName(const LoaderMetadata& meta) noexcept;
    static bool validateSize(const LoaderMetadata& meta) noexcept;
    static bool validateHash(const LoaderMetadata& meta) noexcept;

private:
    LoaderMetadata m_reference;

    bool hasValue(std::string_view val) const noexcept;
};

} // namespace mbootcore
