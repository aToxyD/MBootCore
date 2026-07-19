#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace mbootcore {
namespace firmware {

class FirmwarePackage {
public:
    explicit FirmwarePackage(PackageMetadata metadata);
    ~FirmwarePackage() = default;

    // No copy, move only
    FirmwarePackage(const FirmwarePackage&) = delete;
    FirmwarePackage& operator=(const FirmwarePackage&) = delete;
    FirmwarePackage(FirmwarePackage&&) = default;
    FirmwarePackage& operator=(FirmwarePackage&&) = default;

    // Metadata
    const PackageMetadata& metadata() const noexcept { return m_metadata; }
    PackageMetadata& metadata() noexcept { return m_metadata; }

    // Manifest
    const PackageManifest& manifest() const noexcept { return m_manifest; }
    void setManifest(PackageManifest manifest);

    // Images
    Result<void> addImage(FirmwareImage image);
    Result<FirmwareImage> getImage(const std::string& name) const;
    Result<FirmwareImage> getImageByPartition(const std::string& partitionName) const;
    Result<FirmwareImage> getImageByType(ImageType type) const;
    bool hasImage(const std::string& name) const noexcept;
    size_t imageCount() const noexcept { return m_images.size(); }
    const std::vector<FirmwareImage>& images() const noexcept { return m_images; }
    std::vector<FirmwareImage>& images() noexcept { return m_images; }

    // Dependencies
    const std::vector<FirmwareDependency>& dependencies() const noexcept { return m_manifest.dependencies; }
    void addDependency(FirmwareDependency dep);

    // Identity
    std::string packageId() const;

    // Validation shortcut
    ValidationResult validateBasic() const;

private:
    PackageMetadata m_metadata;
    PackageManifest m_manifest;
    std::vector<FirmwareImage> m_images;
};

} // namespace firmware
} // namespace mbootcore
