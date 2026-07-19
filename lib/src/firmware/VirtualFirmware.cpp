#include <mbootcore/firmware/VirtualFirmware.hpp>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace mbootcore {
namespace firmware {

namespace {

std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

std::string randomUuid() {
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFF);
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << dist(rng()) << dist(rng()) << "-"
       << dist(rng()) << "-"
       << dist(rng()) << "-"
       << dist(rng()) << "-"
       << dist(rng()) << dist(rng()) << dist(rng());
    return ss.str();
}

ByteBuffer computePlaceholderFingerprint(const ByteBuffer& data) {
    uint8_t xorSum = 0;
    for (auto b : data) { xorSum ^= b; }
    return ByteBuffer(32, xorSum);
}

} // anonymous namespace

// ============================================================
// VirtualPackageGenerator
// ============================================================

ByteBuffer VirtualPackageGenerator::createImageData(ImageType type, size_t size) {
    ByteBuffer data(size);
    std::uniform_int_distribution<int> byteDist(0, 255);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(byteDist(rng()));
    }
    // Make data deterministic-ish based on type
    data[0] = static_cast<uint8_t>(type);
    return data;
}

std::string VirtualPackageGenerator::makeUuid() {
    return randomUuid();
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generate(const PackageTemplate& tmpl) {
    m_generatedCount++;
    std::string uuid = tmpl.packageUuid.empty() ? makeUuid() : tmpl.packageUuid;

    PackageMetadata meta;
    meta.vendor = tmpl.vendor;
    meta.platform = tmpl.platform;
    meta.chipset = tmpl.chipset;
    meta.protocol = tmpl.protocol;
    meta.mode = tmpl.mode;
    meta.version = tmpl.version;
    meta.packageUuid = uuid;
    meta.buildDate = "2026-06-30";
    meta.author = "VirtualGenerator";

    auto pkg = std::make_unique<FirmwarePackage>(std::move(meta));
    PackageManifest manifest;

    bool hasDuplicate = !tmpl.duplicatePartitions.empty();

    // Add programmer image
    if (tmpl.includeProgrammer) {
        bool missing = std::find(tmpl.missingImages.begin(), tmpl.missingImages.end(),
                                  "prog") != tmpl.missingImages.end();
        if (!missing) {
            auto data = createImageData(ImageType::Programmer, 8192);
            FirmwareImageInfo info;
            info.name = "prog";
            info.type = ImageType::Programmer;
            info.format = ImageFormat::Raw;
            info.partitionName = "prog";
            info.size = data.size();
            info.sourceFile = "prog.mbn";
            
            if (!tmpl.badHashes) {
                info.expectedHash = computePlaceholderFingerprint(data);
            } else {
                info.expectedHash = ByteBuffer(32, 0xBB);
            }

            FirmwareImage image;
            image.info = std::move(info);
            image.data = std::move(data);
            image.dataLoaded = true;
            (void)pkg->addImage(std::move(image));
        }
    }

    // Add GPT image
    if (tmpl.includeGpt) {
        bool missing = std::find(tmpl.missingImages.begin(), tmpl.missingImages.end(),
                                  "gpt") != tmpl.missingImages.end();
        if (!missing) {
            auto data = createImageData(ImageType::GPT, 17408);
            FirmwareImageInfo info;
            info.name = "gpt";
            info.type = ImageType::GPT;
            info.format = ImageFormat::Raw;
            info.partitionName = "gpt_primary";
            info.size = data.size();
            info.sourceFile = "gpt.bin";
            if (!tmpl.badHashes) {
                info.expectedHash = computePlaceholderFingerprint(data);
            } else {
                info.expectedHash = ByteBuffer(32, 0xBB);
            }

            FirmwareImage image;
            image.info = std::move(info);
            image.data = std::move(data);
            image.dataLoaded = true;
            (void)pkg->addImage(std::move(image));
        }
    }

    // Add standard images
    for (size_t i = 0; i < tmpl.imageCount; ++i) {
        std::string name;
        ImageType type;
        switch (i) {
        case 0: name = "boot"; type = ImageType::Boot; break;
        case 1: name = "system"; type = ImageType::System; break;
        case 2: name = "vendor"; type = ImageType::Vendor; break;
        case 3: name = "userdata"; type = ImageType::Userdata; break;
        case 4: name = "recovery"; type = ImageType::Recovery; break;
        default: name = "custom_" + std::to_string(i); type = ImageType::Custom; break;
        }

        bool missing = std::find(tmpl.missingImages.begin(), tmpl.missingImages.end(),
                                  name) != tmpl.missingImages.end();
        if (missing) continue;

        auto data = createImageData(type, 4096);
        FirmwareImageInfo info;
        info.name = name;
        info.type = type;
        info.format = ImageFormat::Raw;
        info.partitionName = name;
        info.size = data.size();
        info.sourceFile = name + ".bin";
        if (!tmpl.badHashes) {
            info.expectedHash = computePlaceholderFingerprint(data);
        } else {
            info.expectedHash = ByteBuffer(32, 0xBB);
        }

        // Handle duplicate partitions
        if (hasDuplicate && i == 1 && !tmpl.duplicatePartitions.empty()) {
            info.partitionName = tmpl.duplicatePartitions[0];
        }

        FirmwareImage image;
        image.info = std::move(info);
        image.data = std::move(data);
        image.dataLoaded = true;
        (void)pkg->addImage(std::move(image));
    }

    // Handle missing dependencies
    for (const auto& depName : tmpl.missingDependencies) {
        FirmwareDependency dep;
        dep.id = depName;
        dep.name = depName;
        dep.required = true;
        dep.resolved = false;
        pkg->addDependency(dep);
    }

    // Handle version mismatch
    if (tmpl.versionTooLow) {
        pkg->metadata().version = FirmwareVersion{0, 0, 1};
    }

    // Handle unsupported vendor
    if (tmpl.unsupportedVendor) {
        pkg->metadata().vendor = "UNKNOWN_VENDOR";
    }

    // Handle unsupported protocol
    if (tmpl.unsupportedProtocol) {
        pkg->metadata().protocol = "unknown_protocol";
    }

    // Handle corrupt manifest
    if (tmpl.corruptManifest) {
        pkg->metadata().vendor.clear();
    }

    // Collect image infos into manifest
    for (const auto& img : pkg->images()) {
        manifest.images.push_back(img.info);
    }

    // Add dependencies to manifest
    manifest.dependencies = pkg->manifest().dependencies;

    pkg->setManifest(std::move(manifest));

    return pkg;
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateGoodPackage() {
    PackageTemplate tmpl;
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateMissingImage(const std::string& image) {
    PackageTemplate tmpl;
    tmpl.missingImages.push_back(image);
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateBadHash() {
    PackageTemplate tmpl;
    tmpl.badHashes = true;
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateCorruptManifest() {
    PackageTemplate tmpl;
    tmpl.corruptManifest = true;
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateUnsupportedVendor() {
    PackageTemplate tmpl;
    tmpl.unsupportedVendor = true;
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateDuplicatePartitions() {
    PackageTemplate tmpl;
    tmpl.duplicatePartitions.push_back("boot");
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateVersionMismatch() {
    PackageTemplate tmpl;
    tmpl.versionTooLow = true;
    return generate(tmpl);
}

std::unique_ptr<FirmwarePackage> VirtualPackageGenerator::generateMissingDependency() {
    PackageTemplate tmpl;
    tmpl.missingDependencies.push_back("dep_firehose");
    return generate(tmpl);
}

// ============================================================
// VirtualPackageReader
// ============================================================

VirtualPackageReader::VirtualPackageReader(VirtualPackageGenerator& generator)
    : m_generator(&generator) {
}

Result<std::unique_ptr<FirmwarePackage>> VirtualPackageReader::read(const std::string& path) {
    auto it = m_packages.find(path);
    if (it != m_packages.end()) {
        // Return a copy by generating a new one
        auto copy = m_generator->generateGoodPackage();
        return copy;
    }
    return static_cast<ErrorCode>(FirmwareError::PackageNotFound);
}

bool VirtualPackageReader::canRead(const std::string& path) const {
    return m_packages.find(path) != m_packages.end();
}

void VirtualPackageReader::setPackage(const std::string& key,
                                       std::unique_ptr<FirmwarePackage> pkg) {
    m_packages[key] = std::move(pkg);
}

void VirtualPackageReader::clear() {
    m_packages.clear();
}

// ============================================================
// VirtualFirmwareRepository
// ============================================================

VirtualFirmwareRepository::VirtualFirmwareRepository() {
}

std::unique_ptr<FirmwarePackage> VirtualFirmwareRepository::getGoodPackage() {
    return m_generator.generateGoodPackage();
}

std::unique_ptr<FirmwarePackage> VirtualFirmwareRepository::getCorruptPackage() {
    return m_generator.generateCorruptManifest();
}

std::unique_ptr<FirmwarePackage> VirtualFirmwareRepository::getUnsupportedPackage() {
    return m_generator.generateUnsupportedVendor();
}

} // namespace firmware
} // namespace mbootcore
