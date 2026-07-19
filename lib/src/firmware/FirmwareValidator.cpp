#include <mbootcore/firmware/FirmwareValidator.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/security/SecurityTypes.hpp>
#include <algorithm>
#include <set>
#include <cstring>
#include <cctype>

#ifndef MBOOTCORE_ENABLE_CRYPTO
#include <atomic>
#endif

namespace mbootcore {
namespace firmware {

namespace {

std::string vendorToString(discovery::Vendor v) {
    switch (v) {
    case discovery::Vendor::Qualcomm:   return "QUALCOMM";
    case discovery::Vendor::MediaTek:   return "MEDIATEK";
    case discovery::Vendor::UNISOC:     return "UNISOC";
    case discovery::Vendor::Samsung:    return "SAMSUNG";
    case discovery::Vendor::Rockchip:   return "ROCKCHIP";
    case discovery::Vendor::Spreadtrum: return "SPREADTRUM";
    case discovery::Vendor::Apple:      return "APPLE";
    case discovery::Vendor::Google:     return "GOOGLE";
    case discovery::Vendor::Huawei:     return "HUAWEI";
    default:                            return "UNKNOWN";
    }
}

std::string protocolToString(discovery::ProtocolType p) {
    switch (p) {
    case discovery::ProtocolType::Sahara:        return "SAHARA";
    case discovery::ProtocolType::Firehose:      return "FIREHOSE";
    case discovery::ProtocolType::Fastboot:      return "FASTBOOT";
    case discovery::ProtocolType::MediaTekBROM:  return "MEDIATEKBROM";
    case discovery::ProtocolType::MediaTekDA:    return "MEDIATEKDA";
    case discovery::ProtocolType::UNISOCBootROM: return "UNISOCBOOTROM";
    case discovery::ProtocolType::UNISOCFDL:     return "UNISOCFDL";
    case discovery::ProtocolType::USBStream:     return "USBSTREAM";
    default:                                     return "UNKNOWN";
    }
}

} // anonymous namespace

ValidationResult FirmwareValidator::validate(const FirmwarePackage& pkg, ValidationLevel level) const {
    ValidationResult result;
    result.valid = true;

    auto manifestResult = validateManifest(pkg);
    if (!manifestResult.valid) {
        result.valid = false;
        result.errors.insert(result.errors.end(),
            manifestResult.errors.begin(), manifestResult.errors.end());
    }

    if (level >= ValidationLevel::Basic) {
        auto depResult = validateDependencies(pkg);
        if (!depResult.valid) {
            result.valid = false;
            result.errors.insert(result.errors.end(),
                depResult.errors.begin(), depResult.errors.end());
        }
        result.warnings.insert(result.warnings.end(),
            depResult.warnings.begin(), depResult.warnings.end());
    }

    if (level >= ValidationLevel::Full) {
        auto imgResult = validateImages(pkg);
        if (!imgResult.valid) {
            result.valid = false;
            result.errors.insert(result.errors.end(),
                imgResult.errors.begin(), imgResult.errors.end());
        }

        auto intResult = validateIntegrity(pkg);
        if (!intResult.valid) {
            result.valid = false;
            result.errors.insert(result.errors.end(),
                intResult.errors.begin(), intResult.errors.end());
        }
    }

    return result;
}

ValidationResult FirmwareValidator::validateForDevice(
    const FirmwarePackage& pkg,
    const discovery::DeviceDescriptor& device) const {

    ValidationResult result;
    result.valid = true;

    auto baseResult = validate(pkg, ValidationLevel::Full);
    if (!baseResult.valid) {
        result.valid = false;
        result.errors.insert(result.errors.end(),
            baseResult.errors.begin(), baseResult.errors.end());
    }

    if (!pkg.metadata().vendor.empty()) {
        std::string pkgVendor = pkg.metadata().vendor;
        std::transform(pkgVendor.begin(), pkgVendor.end(), pkgVendor.begin(), ::toupper);
        std::string devVendor = vendorToString(device.vendor);
        if (pkgVendor != devVendor && pkgVendor != "ANY" && pkgVendor != "GENERIC") {
            result.valid = false;
            ValidationError err;
            err.error = FirmwareError::UnsupportedVendor;
            err.message = "Package vendor '" + pkg.metadata().vendor + "' does not match device vendor '" + devVendor + "'";
            result.errors.push_back(std::move(err));
        }
    }

    if (!pkg.metadata().protocol.empty()) {
        std::string pkgProto = pkg.metadata().protocol;
        std::transform(pkgProto.begin(), pkgProto.end(), pkgProto.begin(), ::toupper);
        if (pkgProto != "ANY" && pkgProto != "GENERIC") {
            std::string devProto = protocolToString(device.protocolHint);
            if (pkgProto != devProto) {
                result.warnings.push_back(
                    "Package protocol '" + pkg.metadata().protocol
                    + "' differs from device protocol '" + devProto + "'");
            }
        }
    }

    return result;
}

ValidationResult FirmwareValidator::validateDependencies(const FirmwarePackage& pkg) const {
    ValidationResult result;
    result.valid = true;

    for (const auto& dep : pkg.dependencies()) {
        if (dep.required && !dep.resolved) {
            result.valid = false;
            ValidationError err;
            err.error = FirmwareError::DependencyConflict;
            err.message = "Required dependency not resolved: " + dep.id;
            result.errors.push_back(std::move(err));
        }
    }

    return result;
}

ValidationResult FirmwareValidator::validateIntegrity(const FirmwarePackage& pkg) const {
    ValidationResult result;
    result.valid = true;

#ifndef MBOOTCORE_ENABLE_CRYPTO
    {
        static std::atomic<bool> warned{false};
        if (!warned.exchange(true) && m_logger) {
            m_logger->warn("FirmwareValidator",
                "Cryptographic firmware verification is disabled. "
                "Firmware integrity is currently verified using a simple XOR checksum. "
                "This detects accidental corruption only and provides NO protection "
                "against malicious modification. "
                "Enable MBOOTCORE_ENABLE_CRYPTO for production deployments.");
        }
    }
#endif

    for (const auto& image : pkg.images()) {
        if (!image.dataLoaded && !image.info.sourceFile.empty()) {
            result.warnings.push_back("Image data not loaded: " + image.info.name);
            continue;
        }
        if (!verifyHash(image)) {
            result.valid = false;
            ValidationError err;
            err.error = FirmwareError::HashMismatch;
            err.message = "Hash mismatch for image: " + image.info.name;
            result.errors.push_back(std::move(err));
        }
    }

    return result;
}

ValidationResult FirmwareValidator::validateManifest(const FirmwarePackage& pkg) const {
    ValidationResult result;
    result.valid = true;

    if (pkg.metadata().vendor.empty()) {
        ValidationError err;
        err.error = FirmwareError::InvalidManifest;
        err.message = "Missing vendor";
        result.errors.push_back(std::move(err));
        result.valid = false;
    }
    if (pkg.metadata().platform.empty()) {
        ValidationError err;
        err.error = FirmwareError::InvalidManifest;
        err.message = "Missing platform";
        result.errors.push_back(std::move(err));
        result.valid = false;
    }

    for (const auto& img : pkg.images()) {
        if (img.info.name.empty()) {
            ValidationError err;
            err.error = FirmwareError::InvalidManifest;
            err.message = "Image with empty name";
            result.errors.push_back(std::move(err));
            result.valid = false;
        }
    }

    std::set<std::string> partitions;
    for (const auto& img : pkg.images()) {
        if (!img.info.partitionName.empty()) {
            if (!partitions.insert(img.info.partitionName).second) {
                ValidationError err;
                err.error = FirmwareError::DuplicatePartition;
                err.message = "Duplicate partition: " + img.info.partitionName;
                result.errors.push_back(std::move(err));
                result.valid = false;
            }
        }
    }

    return result;
}

ValidationResult FirmwareValidator::validateImages(const FirmwarePackage& pkg) const {
    ValidationResult result;
    result.valid = true;

    for (const auto& img : pkg.images()) {
        if (img.data.empty() && !img.info.sourceFile.empty()) {
            result.warnings.push_back("Image has no data: " + img.info.name);
        }
    }

    return result;
}

bool FirmwareValidator::verifyHash(const FirmwareImage& image) const {
    if (image.info.expectedHash.empty()) {
        return true;
    }
    if (image.data.empty()) {
        return false;
    }
    uint8_t xorSum = 0;
    for (auto b : image.data) {
        xorSum ^= b;
    }
    ByteBuffer computed(image.info.expectedHash.size(), xorSum);
    return computed == image.info.expectedHash;
}

bool FirmwareValidator::verifyHashWithProvider(
    const FirmwareImage& image,
    const security::IHashProvider& provider) const {

    if (image.info.expectedHash.empty()) {
        return true;
    }
    if (image.data.empty()) {
        return false;
    }

    security::HashAlgorithm algo = security::HashAlgorithm::SHA256;
    if (image.info.hashAlgorithm == "SHA512") {
        algo = security::HashAlgorithm::SHA512;
    }

    auto hashResult = provider.hash(image.data, algo);
    if (hashResult.isError()) {
        return false;
    }

    const auto& computed = hashResult.value();
    const auto& expected = image.info.expectedHash;

    if (computed.size() != expected.size()) {
        return false;
    }

    return std::equal(computed.begin(), computed.end(), expected.begin());
}

ValidationResult FirmwareValidator::validateIntegrity(
    const FirmwarePackage& pkg,
    const security::IHashProvider* hashProvider,
    const security::ISignatureVerifier* /*sigVerifier*/) const {

    ValidationResult result;
    result.valid = true;

    for (const auto& image : pkg.images()) {
        if (!image.dataLoaded && !image.info.sourceFile.empty()) {
            result.warnings.push_back("Image data not loaded: " + image.info.name);
            continue;
        }

        if (hashProvider && !image.info.expectedHash.empty()) {
            if (!verifyHashWithProvider(image, *hashProvider)) {
                result.valid = false;
                ValidationError err;
                err.error = FirmwareError::HashMismatch;
                err.message = "Cryptographic hash mismatch for image: " + image.info.name;
                result.errors.push_back(std::move(err));
            }
        } else if (!verifyHash(image)) {
            result.valid = false;
            ValidationError err;
            err.error = FirmwareError::HashMismatch;
            err.message = "Hash mismatch for image: " + image.info.name;
            result.errors.push_back(std::move(err));
        }
    }

    return result;
}

} // namespace firmware
} // namespace mbootcore
