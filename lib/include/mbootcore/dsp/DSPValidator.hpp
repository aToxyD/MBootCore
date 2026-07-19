#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

struct ValidationError {
    std::string field;
    std::string message;
    DSPError error{DSPError::ValidationFailed};
    bool critical{true};
};

struct ValidationReport {
    bool valid{false};
    std::vector<ValidationError> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> info;
    size_t filesChecked{0};
    size_t checksumsVerified{0};
    size_t loadersVerified{0};
    std::string packageId;
    DSPVersion packageVersion;
};

class DSPValidator {
public:
    DSPValidator();

    ValidationReport validate(const std::string& packagePath, DSPValidationLevel level = DSPValidationLevel::Full);
    ValidationReport validateMetadata(const DSPPackageMetadata& metadata);
    ValidationReport validateManifest(const std::string& packagePath);
    ValidationReport validateChecksums(const std::string& packagePath);
    ValidationReport validateLoaders(const std::string& packagePath, const DSPPackageMetadata& metadata);
    ValidationReport validateSchema(const std::string& packagePath);
    ValidationReport validateDependencies(const DSPPackageMetadata& metadata,
        const std::vector<DSPPackageMetadata>& installed);
    ValidationReport validateSignature(const std::string& packagePath);
    ValidationReport validateVersionCompatibility(const DSPPackageMetadata& metadata,
        const std::string& coreVersion);

    void setValidationLevel(DSPValidationLevel level) noexcept;
    DSPValidationLevel validationLevel() const noexcept;

private:
    DSPValidationLevel m_level{DSPValidationLevel::Full};

    bool checkFileExists(const std::string& path) const;
    bool checkJsonSchema(const std::string& jsonPath, const std::string& schemaName) const;
    uint32_t computeFileChecksum(const std::string& path) const;
    bool verifyChecksum(const std::string& path, uint32_t expected) const;
};

} // namespace dsp
} // namespace mbootcore
