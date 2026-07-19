#include "mbootcore/loader/LoaderValidator.hpp"

namespace mbootcore {

LoaderValidator::LoaderValidator(const LoaderMetadata& reference)
    : m_reference(reference) {}

bool LoaderValidator::hasValue(std::string_view val) const noexcept {
    return !val.empty();
}

bool LoaderValidator::hasRequiredMetadata(const LoaderMetadata& meta) const noexcept {
    if (meta.vendor.empty()) return false;
    if (meta.protocol.empty()) return false;
    if (meta.loaderSize == 0) return false;
    return true;
}

bool LoaderValidator::isHashValid(const LoaderMetadata& meta) const noexcept {
    if (meta.hash.empty()) return true;
    return validateHash(meta);
}

bool LoaderValidator::isCompatible(const LoaderMetadata& meta) const noexcept {
    if (!m_reference.vendor.empty() && meta.vendor != m_reference.vendor) return false;
    if (!m_reference.protocol.empty() && meta.protocol != m_reference.protocol) return false;
    return true;
}

bool LoaderValidator::isCorrupted(const LoaderMetadata& meta) const noexcept {
    if (meta.loaderSize == 0) return true;
    if (meta.hash.empty()) return false;
    return !validateHash(meta);
}

std::vector<std::string> LoaderValidator::validationErrors(const LoaderMetadata& meta) const {
    std::vector<std::string> errors;
    if (meta.vendor.empty()) errors.push_back("Missing vendor");
    if (meta.protocol.empty()) errors.push_back("Missing protocol");
    if (meta.loaderSize == 0) errors.push_back("Missing or zero loader size");
    if (meta.capabilities.empty()) errors.push_back("No capabilities declared");
    return errors;
}

bool LoaderValidator::validateName(const LoaderMetadata& meta) noexcept {
    (void)meta;
    return true;
}

bool LoaderValidator::validateSize(const LoaderMetadata& meta) noexcept {
    return meta.loaderSize > 0;
}

bool LoaderValidator::validateHash(const LoaderMetadata& meta) noexcept {
    return !meta.hash.empty();
}

} // namespace mbootcore
