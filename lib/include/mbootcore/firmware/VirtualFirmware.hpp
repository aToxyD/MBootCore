#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/IFirmwareReader.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareValidator.hpp>
#include <mbootcore/firmware/FlashPlan.hpp>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace mbootcore {
namespace firmware {

struct PackageTemplate {
    std::string vendor{"QUALCOMM"};
    std::string platform{"SM8450"};
    std::string chipset{"SM8450"};
    std::string protocol{"sahara"};
    std::string mode{"ufs"};
    FirmwareVersion version{1, 0, 0};
    std::string packageUuid;
    bool includeProgrammer{true};
    bool includeGpt{true};
    size_t imageCount{3};
    
    // Failure injection
    std::vector<std::string> missingImages;
    std::vector<std::string> duplicatePartitions;
    bool corruptManifest{false};
    bool badHashes{false};
    bool missingManifest{false};
    bool unsupportedVendor{false};
    bool unsupportedProtocol{false};
    bool versionTooLow{false};
    std::vector<std::string> missingDependencies;
};

class VirtualPackageGenerator {
public:
    VirtualPackageGenerator() = default;
    
    std::unique_ptr<FirmwarePackage> generate(const PackageTemplate& tmpl);
    std::unique_ptr<FirmwarePackage> generateGoodPackage();
    std::unique_ptr<FirmwarePackage> generateMissingImage(const std::string& image);
    std::unique_ptr<FirmwarePackage> generateBadHash();
    std::unique_ptr<FirmwarePackage> generateCorruptManifest();
    std::unique_ptr<FirmwarePackage> generateUnsupportedVendor();
    std::unique_ptr<FirmwarePackage> generateDuplicatePartitions();
    std::unique_ptr<FirmwarePackage> generateVersionMismatch();
    std::unique_ptr<FirmwarePackage> generateMissingDependency();
    
    size_t generatedCount() const noexcept { return m_generatedCount; }
    void reset() { m_generatedCount = 0; }

private:
    ByteBuffer createImageData(ImageType type, size_t size = 4096);
    std::string makeUuid();
    size_t m_generatedCount{0};
};

class VirtualPackageReader : public IFirmwareReader {
public:
    explicit VirtualPackageReader(VirtualPackageGenerator& generator);
    
    Result<std::unique_ptr<FirmwarePackage>> read(const std::string& path) override;
    bool canRead(const std::string& path) const override;
    std::string name() const override { return "VirtualReader"; }
    
    void setPackage(const std::string& key, std::unique_ptr<FirmwarePackage> pkg);
    void clear();

private:
    VirtualPackageGenerator* m_generator;
    std::unordered_map<std::string, std::unique_ptr<FirmwarePackage>> m_packages;
};

class VirtualFirmwareRepository {
public:
    VirtualFirmwareRepository();
    
    // Pre-built packages
    std::unique_ptr<FirmwarePackage> getGoodPackage();
    std::unique_ptr<FirmwarePackage> getCorruptPackage();
    std::unique_ptr<FirmwarePackage> getUnsupportedPackage();
    
    // Generator access
    VirtualPackageGenerator& generator() noexcept { return m_generator; }
    const VirtualPackageGenerator& generator() const noexcept { return m_generator; }

private:
    VirtualPackageGenerator m_generator;
};

} // namespace firmware
} // namespace mbootcore
