#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/security/interfaces/IHashProvider.hpp>
#include <mbootcore/security/interfaces/ISignatureVerifier.hpp>
#include <mbootcore/security/interfaces/IIntegrityVerifier.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <string>
#include <vector>

namespace mbootcore {
namespace firmware {

class FirmwareValidator {
public:
    void setLogger(ILogger* logger) noexcept { m_logger = logger; }

    ValidationResult validate(const FirmwarePackage& pkg, ValidationLevel level = ValidationLevel::Full) const;
    ValidationResult validateForDevice(const FirmwarePackage& pkg,
                                       const discovery::DeviceDescriptor& device) const;
    ValidationResult validateDependencies(const FirmwarePackage& pkg) const;
    ValidationResult validateIntegrity(const FirmwarePackage& pkg) const;
    ValidationResult validateIntegrity(
        const FirmwarePackage& pkg,
        const security::IHashProvider* hashProvider,
        const security::ISignatureVerifier* sigVerifier) const;
    ValidationResult validateManifest(const FirmwarePackage& pkg) const;
    ValidationResult validateImages(const FirmwarePackage& pkg) const;

private:
    bool verifyHash(const FirmwareImage& image) const;
    bool verifyHashWithProvider(const FirmwareImage& image,
                                const security::IHashProvider& provider) const;

    ILogger* m_logger{nullptr};
};

} // namespace firmware
} // namespace mbootcore
