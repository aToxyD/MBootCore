#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <algorithm>
#include <sstream>

namespace mbootcore {
namespace firmware {

FirmwarePackage::FirmwarePackage(PackageMetadata metadata)
    : m_metadata(std::move(metadata)) {
}

void FirmwarePackage::setManifest(PackageManifest manifest) {
    m_manifest = std::move(manifest);
}

Result<void> FirmwarePackage::addImage(FirmwareImage image) {
    auto it = std::find_if(m_images.begin(), m_images.end(),
        [&](const FirmwareImage& existing) {
            return existing.info.name == image.info.name;
        });
    if (it != m_images.end()) {
        return static_cast<ErrorCode>(FirmwareError::DuplicatePartition);
    }
    m_images.push_back(std::move(image));
    return {};
}

Result<FirmwareImage> FirmwarePackage::getImage(const std::string& name) const {
    auto it = std::find_if(m_images.begin(), m_images.end(),
        [&](const FirmwareImage& img) { return img.info.name == name; });
    if (it == m_images.end()) {
        return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
    }
    return *it;
}

Result<FirmwareImage> FirmwarePackage::getImageByPartition(const std::string& partitionName) const {
    auto it = std::find_if(m_images.begin(), m_images.end(),
        [&](const FirmwareImage& img) { return img.info.partitionName == partitionName; });
    if (it == m_images.end()) {
        return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
    }
    return *it;
}

Result<FirmwareImage> FirmwarePackage::getImageByType(ImageType type) const {
    auto it = std::find_if(m_images.begin(), m_images.end(),
        [&](const FirmwareImage& img) { return img.info.type == type; });
    if (it == m_images.end()) {
        return static_cast<ErrorCode>(FirmwareError::ImageNotFound);
    }
    return *it;
}

bool FirmwarePackage::hasImage(const std::string& name) const noexcept {
    return std::any_of(m_images.begin(), m_images.end(),
        [&](const FirmwareImage& img) { return img.info.name == name; });
}

void FirmwarePackage::addDependency(FirmwareDependency dep) {
    m_manifest.dependencies.push_back(std::move(dep));
}

std::string FirmwarePackage::packageId() const {
    return m_metadata.vendor + "_" + m_metadata.platform + "_" + m_metadata.version.toString();
}

ValidationResult FirmwarePackage::validateBasic() const {
    ValidationResult result;
    result.valid = true;

    if (m_metadata.vendor.empty()) {
        result.valid = false;
        result.errors.push_back({FirmwareError::InvalidManifest, "Missing vendor", ""});
    }
    if (m_metadata.platform.empty()) {
        result.valid = false;
        result.errors.push_back({FirmwareError::InvalidManifest, "Missing platform", ""});
    }

    for (const auto& img : m_images) {
        if (img.info.name.empty()) {
            result.valid = false;
            result.errors.push_back({FirmwareError::InvalidManifest, "Image with empty name", ""});
        }
    }

    if (m_images.empty()) {
        result.warnings.push_back("Package contains no images");
    }

    return result;
}

} // namespace firmware
} // namespace mbootcore
