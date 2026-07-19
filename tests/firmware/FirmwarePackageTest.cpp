#include <catch2/catch_test_macros.hpp>

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareReaders.hpp>
#include <mbootcore/firmware/FirmwareValidator.hpp>
#include <mbootcore/firmware/FirmwareResolver.hpp>
#include <mbootcore/firmware/ImageEngine.hpp>
#include <mbootcore/firmware/FlashPlan.hpp>
#include <mbootcore/firmware/FirmwareExecutor.hpp>
#include <mbootcore/firmware/VirtualFirmware.hpp>
#include <mbootcore/job/VirtualJobDevice.hpp>
#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/job/JobPipeline.hpp>
#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/domain/Error.hpp>

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

using namespace mbootcore;
using namespace mbootcore::firmware;
using namespace mbootcore::job;

namespace fs = std::filesystem;

namespace {

class NullTestLogger : public ILogger {
public:
    void log(LogLevel, std::string_view, const std::string&) override {}
    void setLevel(LogLevel) override {}
    LogLevel level() const noexcept override { return LogLevel::Info; }
};

static std::string makeTempDir() {
    for (int i = 0; i < 100; ++i) {
        auto p = fs::temp_directory_path() / ("mboot_fw_test_" + std::to_string(i));
        if (!fs::exists(p)) {
            fs::create_directories(p);
            return p.string();
        }
    }
    return (fs::temp_directory_path() / "mboot_fw_test_fallback").string();
}

}

TEST_CASE("FirmwarePackageTest", "[firmware]") {

SECTION("testVersionDefault") {
    FirmwareVersion v;
    REQUIRE(v.major == uint32_t(0));
    REQUIRE(v.minor == uint32_t(0));
    REQUIRE(v.patch == uint32_t(0));
}

SECTION("testVersionComparison") {
    FirmwareVersion a{1, 0, 0};
    FirmwareVersion b{1, 0, 0};
    FirmwareVersion c{2, 0, 0};
    REQUIRE(a == b);
    REQUIRE(a < c);
    REQUIRE(!(c < a));
}

SECTION("testVersionToString") {
    FirmwareVersion v{2, 5, 1};
    REQUIRE(v.toString() == "2.5.1");
}

SECTION("testImageInfoDefaults") {
    FirmwareImageInfo info;
    REQUIRE(info.name == std::string());
    REQUIRE(static_cast<int>(info.type) == static_cast<int>(ImageType::Custom));
    REQUIRE(static_cast<int>(info.format) == static_cast<int>(ImageFormat::Raw));
    REQUIRE(info.offset == uint64_t(0));
    REQUIRE(info.size == uint64_t(0));
    REQUIRE(info.isExecutable == false);
}

SECTION("testPackageMetadataDefaults") {
    PackageMetadata meta;
    REQUIRE(meta.vendor == std::string());
    REQUIRE(meta.version.major == uint32_t(0));
    REQUIRE(meta.packageSize == uint64_t(0));
}

SECTION("testFlashStepDefaults") {
    FlashStep step;
    REQUIRE(static_cast<int>(step.type) == static_cast<int>(FlashStepType::Custom));
    REQUIRE(step.offset == uint64_t(0));
    REQUIRE(step.requireBackup == false);
    REQUIRE(step.optional == false);
}

SECTION("testFlashPlanDefaults") {
    FlashPlan plan;
    REQUIRE(plan.steps.empty());
    REQUIRE(plan.requireProgrammer == false);
    REQUIRE(plan.requireGptUpdate == false);
    REQUIRE(plan.totalBytes == uint64_t(0));
}

SECTION("testValidationErrorDefaults") {
    ValidationError err;
    REQUIRE(static_cast<int>(err.error) == static_cast<int>(FirmwareError::ValidationFailed));
    REQUIRE(err.message == std::string());
}

SECTION("testDependencyDefaults") {
    FirmwareDependency dep;
    REQUIRE(dep.id == std::string());
    REQUIRE(dep.required == true);
    REQUIRE(dep.resolved == false);
}

SECTION("testSignatureDefaults") {
    FirmwareSignature sig;
    REQUIRE(sig.algorithm == std::string("SHA256"));
    REQUIRE(sig.data.empty());
    REQUIRE(sig.valid == false);
}

SECTION("testFirmwareErrorToString") {
    auto code = static_cast<ErrorCode>(FirmwareError::InvalidFormat);
    auto str = toString(code);
    REQUIRE(!std::string(str).empty());
}

SECTION("testPackageCreate") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    REQUIRE(pkg.metadata().vendor == std::string("QUALCOMM"));
    REQUIRE(pkg.metadata().platform == std::string("SM8450"));
    REQUIRE(pkg.imageCount() == size_t(0));
}

SECTION("testPackageAddImage") {
    PackageMetadata meta;
    meta.vendor = "QCOM";
    auto pkg = FirmwarePackage(std::move(meta));

    FirmwareImage image;
    image.info.name = "boot";
    image.info.partitionName = "boot";
    image.info.type = ImageType::Boot;
    image.data = ByteBuffer(256, 0xAA);
    image.dataLoaded = true;

    auto result = pkg.addImage(std::move(image));
    REQUIRE(result.isOk());
    REQUIRE(pkg.imageCount() == size_t(1));
}

SECTION("testPackageAddDuplicateImageFails") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    
    FirmwareImage img1;
    img1.info.name = "boot";
    REQUIRE(pkg.addImage(std::move(img1)).isOk());

    FirmwareImage img2;
    img2.info.name = "boot";
    auto result = pkg.addImage(std::move(img2));
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::FirmwareDuplicatePartition));
}

SECTION("testPackageGetImage") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    
    FirmwareImage image;
    image.info.name = "system";
    image.data = ByteBuffer(128, 0xBB);
    pkg.addImage(std::move(image));

    auto result = pkg.getImage("system");
    REQUIRE(result.isOk());
    REQUIRE(result.value().info.name == std::string("system"));
}

SECTION("testPackageGetImageNotFound") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    auto result = pkg.getImage("nonexistent");
    REQUIRE(result.isError());
}

SECTION("testPackageGetImageByPartition") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    
    FirmwareImage image;
    image.info.name = "boot";
    image.info.partitionName = "boot_a";
    pkg.addImage(std::move(image));

    auto result = pkg.getImageByPartition("boot_a");
    REQUIRE(result.isOk());
    REQUIRE(result.value().info.name == std::string("boot"));
}

SECTION("testPackageGetImageByType") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    
    FirmwareImage img;
    img.info.name = "prog";
    img.info.type = ImageType::Programmer;
    pkg.addImage(std::move(img));

    auto result = pkg.getImageByType(ImageType::Programmer);
    REQUIRE(result.isOk());
    REQUIRE(result.value().info.name == std::string("prog"));
}

SECTION("testPackageHasImage") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    
    FirmwareImage img;
    img.info.name = "boot";
    pkg.addImage(std::move(img));

    REQUIRE(pkg.hasImage("boot"));
    REQUIRE(!pkg.hasImage("nonexistent"));
}

SECTION("testPackageImageCount") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    REQUIRE(pkg.imageCount() == size_t(0));
    
    FirmwareImage i1; i1.info.name = "a"; pkg.addImage(std::move(i1));
    REQUIRE(pkg.imageCount() == size_t(1));
    
    FirmwareImage i2; i2.info.name = "b"; pkg.addImage(std::move(i2));
    REQUIRE(pkg.imageCount() == size_t(2));
}

SECTION("testPackageAddDependency") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    REQUIRE(pkg.dependencies().empty());

    FirmwareDependency dep;
    dep.id = "test_dep";
    dep.required = true;
    pkg.addDependency(dep);
    REQUIRE(pkg.dependencies().size() == size_t(1));
    REQUIRE(pkg.dependencies()[0].id == std::string("test_dep"));
}

SECTION("testPackageId") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    meta.version = FirmwareVersion{1, 2, 3};
    auto pkg = FirmwarePackage(std::move(meta));
    REQUIRE(pkg.packageId() == std::string("QUALCOMM_SM8450_1.2.3"));
}

SECTION("testPackageValidateBasicValid") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    
    FirmwareImage img;
    img.info.name = "boot";
    pkg.addImage(std::move(img));

    auto result = pkg.validateBasic();
    REQUIRE(result.valid);
}

SECTION("testPackageValidateBasicEmptyVendor") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    auto result = pkg.validateBasic();
    REQUIRE(!result.valid);
    REQUIRE(!result.errors.empty());
}

SECTION("testPackageValidateBasicEmptyImages") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    auto result = pkg.validateBasic();
    REQUIRE(result.valid);
    REQUIRE(!result.warnings.empty());
}

SECTION("testPackageSetManifest") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    PackageManifest manifest;
    manifest.manifestVersion = "2.0";
    manifest.description = "Test manifest";
    pkg.setManifest(std::move(manifest));
    REQUIRE(pkg.manifest().manifestVersion == std::string("2.0"));
    REQUIRE(pkg.manifest().description == std::string("Test manifest"));
}

SECTION("testPackageMoveSemantics") {
    auto pkg1 = std::make_unique<FirmwarePackage>(PackageMetadata{});
    FirmwareImage img;
    img.info.name = "boot";
    pkg1->addImage(std::move(img));
    REQUIRE(pkg1->imageCount() == size_t(1));

    auto pkg2 = std::move(pkg1);
    REQUIRE(pkg2->imageCount() == size_t(1));
    REQUIRE(pkg1 == nullptr);
}

SECTION("testDirReaderCanReadDir") {
    DirectoryFirmwareReader reader;
    auto tmpDir = makeTempDir();
    REQUIRE(reader.canRead(tmpDir));
    fs::remove_all(tmpDir);
}

SECTION("testDirReaderCannotReadFile") {
    DirectoryFirmwareReader reader;
    REQUIRE(!reader.canRead("nonexistent_path"));
}

SECTION("testDirReaderMissingManifest") {
    DirectoryFirmwareReader reader;
    auto tmpDir = makeTempDir();
    auto result = reader.read(tmpDir);
    REQUIRE(result.isError());
    fs::remove_all(tmpDir);
}

SECTION("testDirReaderValidPackage") {
    auto tmpDir = makeTempDir();
    
    std::ofstream mf(tmpDir + "/manifest.json");
    mf << R"({
        "vendor": "QUALCOMM",
        "platform": "SM8450",
        "chipset": "SM8450",
        "protocol": "sahara",
        "version": "1.0.0",
        "images": []
    })";
    mf.close();

    DirectoryFirmwareReader reader;
    auto result = reader.read(tmpDir);
    REQUIRE(result.isOk());
    auto pkg = std::move(result.value());
    REQUIRE(pkg->metadata().vendor == std::string("QUALCOMM"));
    REQUIRE(pkg->metadata().platform == std::string("SM8450"));
    REQUIRE(pkg->metadata().version.major == uint32_t(1));
    fs::remove_all(tmpDir);
}

SECTION("testDirReaderWithImages") {
    auto tmpDir = makeTempDir();
    
    ByteBuffer testData(256, 0xCC);
    std::ofstream imgFile(tmpDir + "/boot.bin", std::ios::binary);
    imgFile.write(reinterpret_cast<const char*>(testData.data()), testData.size());
    imgFile.close();

    std::ofstream mf(tmpDir + "/manifest.json");
    mf << R"({
        "vendor": "QUALCOMM",
        "platform": "SM8450",
        "version": "1.0.0",
        "images": [
            {"name": "boot", "type": "Boot", "partition": "boot", "file": "boot.bin"}
        ]
    })";
    mf.close();

    DirectoryFirmwareReader reader;
    auto result = reader.read(tmpDir);
    REQUIRE(result.isOk());
    REQUIRE(result.value()->imageCount() == size_t(1));
    REQUIRE(result.value()->images()[0].info.name == std::string("boot"));
    REQUIRE(!result.value()->images()[0].data.empty());
    fs::remove_all(tmpDir);
}

SECTION("testRawReaderCanReadBin") {
    RawFirmwareReader reader;
    REQUIRE(reader.canRead("test.bin"));
    REQUIRE(reader.canRead("test.img"));
    REQUIRE(reader.canRead("test.elf"));
}

SECTION("testRawReaderCanReadElf") {
    RawFirmwareReader reader;
    REQUIRE(reader.canRead("firmware.elf"));
}

SECTION("testRawReaderCannotReadDir") {
    RawFirmwareReader reader;
    auto tmpDir = makeTempDir();
    REQUIRE(!reader.canRead(tmpDir));
    fs::remove_all(tmpDir);
}

SECTION("testRawReaderReadValidFile") {
    auto tmpDir = makeTempDir();
    auto filePath = tmpDir + "/test.bin";
    
    ByteBuffer testData(512, 0xDD);
    std::ofstream f(filePath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(testData.data()), testData.size());
    f.close();

    RawFirmwareReader reader;
    auto result = reader.read(filePath);
    REQUIRE(result.isOk());
    REQUIRE(result.value()->imageCount() == size_t(1));
    REQUIRE(result.value()->images()[0].data.size() == size_t(512));
    fs::remove_all(tmpDir);
}

SECTION("testRawReaderReadNonExistent") {
    RawFirmwareReader reader;
    auto result = reader.read("nonexistent.bin");
    REQUIRE(result.isError());
}

SECTION("testZipReaderCanReadZip") {
    ZipFirmwareReader reader;
    REQUIRE(reader.canRead("package.zip"));
    REQUIRE(!reader.canRead("package.bin"));
}

SECTION("testZipReaderNotYetSupported") {
    ZipFirmwareReader reader;
    auto result = reader.read("test.zip");
    REQUIRE(result.isError());
}

SECTION("testValidateValidPackage") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg, ValidationLevel::Full);
    REQUIRE(result.valid);
}

SECTION("testValidateMissingVendor") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateCorruptManifest();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg, ValidationLevel::Basic);
    REQUIRE(!result.valid);
    bool found = false;
    for (const auto& err : result.errors) {
        if (err.error == FirmwareError::InvalidManifest) found = true;
    }
    REQUIRE(found);
}

SECTION("testValidateMissingPlatform") {
    PackageMetadata meta;
    meta.vendor = "QCOM";
    auto pkg = FirmwarePackage(std::move(meta));
    FirmwareValidator validator;
    auto result = validator.validate(pkg);
    REQUIRE(!result.valid);
}

SECTION("testValidateWithDependencies") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FirmwareDependency dep;
    dep.id = "test";
    dep.required = true;
    dep.resolved = true;
    pkg->addDependency(dep);
    FirmwareValidator validator;
    auto result = validator.validate(*pkg);
    REQUIRE(result.valid);
}

SECTION("testValidateMissingDependency") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateMissingDependency();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg);
    REQUIRE(!result.valid);
    bool found = false;
    for (const auto& err : result.errors) {
        if (err.error == FirmwareError::DependencyConflict) found = true;
    }
    REQUIRE(found);
}

SECTION("testValidateDuplicatePartition") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateDuplicatePartitions();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg);
    REQUIRE(!result.valid);
}

SECTION("testValidateHashMismatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateBadHash();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg, ValidationLevel::Strict);
    REQUIRE(!result.valid);
}

SECTION("testValidateIntegrityNoHash") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    FirmwareImage img;
    img.info.name = "test";
    img.data = ByteBuffer(16, 0xAA);
    img.dataLoaded = true;
    pkg.addImage(std::move(img));
    FirmwareValidator validator;
    auto result = validator.validateIntegrity(pkg);
    REQUIRE(result.valid);
}

SECTION("testValidateForDeviceMatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::Qualcomm;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareValidator validator;
    auto result = validator.validateForDevice(*pkg, device);
    REQUIRE(result.valid);
}

SECTION("testValidateForDeviceVendorMismatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateUnsupportedVendor();
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::Qualcomm;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareValidator validator;
    auto result = validator.validateForDevice(*pkg, device);
    REQUIRE(!result.valid);
}

SECTION("testValidateForDeviceProtocolMismatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    pkg->metadata().protocol = "firehose";
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::Qualcomm;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareValidator validator;
    auto result = validator.validateForDevice(*pkg, device);
    REQUIRE(result.valid);
}

SECTION("testValidateForDeviceAnyVendor") {
    PackageMetadata meta;
    meta.vendor = "ANY";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::MediaTek;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareValidator validator;
    auto result = validator.validateForDevice(pkg, device);
    REQUIRE(result.valid);
}

SECTION("testValidateLevelNone") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg, ValidationLevel::None);
    REQUIRE(result.valid);
}

SECTION("testValidateLevelBasic") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg, ValidationLevel::Basic);
    REQUIRE(result.valid);
}

SECTION("testValidateLevelFull") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FirmwareValidator validator;
    auto result = validator.validate(*pkg, ValidationLevel::Full);
    REQUIRE(result.valid);
}

SECTION("testValidateImageEmptyData") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    FirmwareImage img;
    img.info.name = "empty_img";
    img.info.sourceFile = "missing.bin";
    pkg.addImage(std::move(img));
    FirmwareValidator validator;
    auto result = validator.validate(pkg);
    REQUIRE(result.valid);
}

SECTION("testValidateManifestImageEmptyName") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    FirmwareImage img;
    pkg.addImage(std::move(img));
    FirmwareValidator validator;
    auto result = validator.validateManifest(pkg);
    REQUIRE(!result.valid);
}

SECTION("testResolveBasic") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    
    auto resolver = FirmwareResolver();
    
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::Qualcomm;
    device.protocolHint = discovery::ProtocolType::Sahara;
    
    auto result = resolver.resolve(std::move(pkg), device);
    REQUIRE(result.isOk());
}

SECTION("testResolveWithVendor") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    pkg.metadata().vendor = "QUALCOMM";
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::Qualcomm;
    FirmwareResolver resolver;
    auto& meta = pkg.metadata();
    REQUIRE(meta.vendor == "QUALCOMM");
}

SECTION("testResolveWrongVendorFails") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateUnsupportedVendor();
    discovery::DeviceDescriptor device;
    device.vendor = discovery::Vendor::Qualcomm;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareResolver resolver;
    auto result = resolver.resolve(std::move(pkg), device);
    REQUIRE(result.isError());
}

SECTION("testResolveProtocolMatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    pkg->metadata().protocol = "sahara";
    discovery::DeviceDescriptor device;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareResolver resolver;
    REQUIRE(resolver.matchProtocol(pkg->metadata(), device));
}

SECTION("testResolveProtocolMismatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    pkg->metadata().protocol = "firehose";
    discovery::DeviceDescriptor device;
    device.protocolHint = discovery::ProtocolType::Sahara;
    FirmwareResolver resolver;
    REQUIRE(!resolver.matchProtocol(pkg->metadata(), device));
}

SECTION("testResolveWithProgrammer") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    auto progResult = pkg->getImageByType(ImageType::Programmer);
    REQUIRE(progResult.isOk());
}

SECTION("testResolveWithGpt") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    auto gptResult = pkg->getImageByType(ImageType::GPT);
    REQUIRE(gptResult.isOk());
}

SECTION("testResolveNoProgrammer") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateMissingImage("prog");
    auto progResult = pkg->getImageByType(ImageType::Programmer);
    REQUIRE(progResult.isError());
}

SECTION("testResolveEmptyDependencies") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    REQUIRE(pkg->dependencies().empty());
}

SECTION("testResolvePathNotFound") {
    FirmwareResolver resolver;
    auto result = resolver.resolve("/nonexistent/path");
    REQUIRE(result.isError());
}

SECTION("testImageEngineLoadImageData") {
    auto tmpDir = makeTempDir();
    auto filePath = tmpDir + "/test.bin";
    ByteBuffer data(64, 0xEE);
    std::ofstream f(filePath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    f.close();

    FirmwareImage image;
    image.info.name = "test";
    image.info.sourceFile = "test.bin";
    
    ImageEngine engine;
    auto result = engine.loadImageData(std::move(image), tmpDir);
    REQUIRE(result.isOk());
    REQUIRE(result.value().data.size() == size_t(64));
    fs::remove_all(tmpDir);
}

SECTION("testImageEngineLoadMissingFile") {
    FirmwareImage image;
    image.info.name = "test";
    image.info.sourceFile = "missing.bin";
    ImageEngine engine;
    auto result = engine.loadImageData(std::move(image), "/nonexistent");
    REQUIRE(result.isError());
}

SECTION("testImageEngineVerifyImage") {
    FirmwareImage image;
    image.info.name = "test";
    image.data = ByteBuffer(32, 0xFF);
    image.dataLoaded = true;
    ImageEngine engine;
    auto result = engine.verifyImage(image);
    REQUIRE(result.isOk());
}

SECTION("testImageEngineVerifyAllImages") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    ImageEngine engine;
    auto result = engine.verifyAllImages(*pkg);
    REQUIRE(result.isOk());
}

SECTION("testImageEngineExtractRaw") {
    FirmwareImage image;
    image.data = ByteBuffer(16, 0xAB);
    image.dataLoaded = true;
    ImageEngine engine;
    auto result = engine.extractRaw(image);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == size_t(16));
}

SECTION("testImageEngineExtractToFile") {
    auto tmpDir = makeTempDir();
    auto outputPath = tmpDir + "/output.bin";
    FirmwareImage image;
    image.data = ByteBuffer(32, 0xCD);
    image.dataLoaded = true;
    ImageEngine engine;
    auto result = engine.extractToFile(image, outputPath);
    REQUIRE(result.isOk());
    REQUIRE(fs::exists(outputPath));
    fs::remove_all(tmpDir);
}

SECTION("testImageEngineConvertToRaw") {
    FirmwareImage image;
    image.info.format = ImageFormat::Raw;
    image.data = ByteBuffer(8, 0x12);
    image.dataLoaded = true;
    ImageEngine engine;
    auto result = engine.convertToRaw(image);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == size_t(8));
}

SECTION("testImageEngineVerifyNoData") {
    FirmwareImage image;
    image.info.name = "nodata";
    ImageEngine engine;
    auto result = engine.verifyImage(image);
    REQUIRE(result.isError());
}

SECTION("testFlashPlanGenerateEmpty") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    FlashPlanGenerator gen;
    auto result = gen.generatePlan(pkg);
    REQUIRE(result.isOk());
    REQUIRE(!result.value().requireProgrammer);
    REQUIRE(!result.value().requireGptUpdate);
}

SECTION("testFlashPlanGenerateProgrammer") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto result = planGen.generatePlan(*pkg);
    REQUIRE(result.isOk());
    REQUIRE(result.value().requireProgrammer);
}

SECTION("testFlashPlanGenerateGpt") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto result = planGen.generatePlan(*pkg);
    REQUIRE(result.isOk());
    REQUIRE(result.value().requireGptUpdate);
}

SECTION("testFlashPlanGenerateMultipleImages") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto result = planGen.generatePlan(*pkg);
    REQUIRE(result.isOk());
    REQUIRE(result.value().steps.size() >= 2);
}

SECTION("testFlashPlanGenerateFromResolved") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    ResolvedPackage resolved;
    resolved.package = std::move(pkg);
    resolved.programmerFound = true;
    resolved.gptFound = true;
    FlashPlanGenerator planGen;
    auto plan = planGen.generateFromResolved(resolved);
    REQUIRE(plan.requireProgrammer);
    REQUIRE(plan.requireGptUpdate);
}

SECTION("testFlashPlanOrdering") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto result = planGen.generatePlan(*pkg);
    REQUIRE(result.isOk());
    auto& steps = result.value().steps;
    bool hasProgrammer = false;
    bool hasGpt = false;
    bool hasPartition = false;
    for (const auto& step : steps) {
        if (step.type == FlashStepType::FlashProgrammer) hasProgrammer = true;
        if (step.type == FlashStepType::UpdateGPT) hasGpt = true;
        if (step.type == FlashStepType::FlashPartition) hasPartition = true;
    }
    REQUIRE(hasProgrammer);
    REQUIRE(hasGpt);
    REQUIRE(hasPartition);
}

SECTION("testFlashPlanWithVerifySteps") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto result = planGen.generatePlan(*pkg);
    REQUIRE(result.isOk());
    bool hasVerify = false;
    for (const auto& step : result.value().steps) {
        if (step.type == FlashStepType::VerifyPartition) hasVerify = true;
    }
    REQUIRE(hasVerify);
}

SECTION("testFlashPlanTotalBytes") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto result = planGen.generatePlan(*pkg);
    REQUIRE(result.isOk());
    REQUIRE(result.value().totalBytes > 0);
}

SECTION("testFlashPlanRebootStep") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    FlashPlanGenerator planGen;
    auto plan = planGen.generateFromResolved(ResolvedPackage{});
    bool hasReboot = false;
    for (const auto& step : plan.steps) {
        if (step.type == FlashStepType::Reboot) hasReboot = true;
    }
    REQUIRE(hasReboot);
}

SECTION("testExecutorCreate") {
    FirmwareExecutor executor;
    REQUIRE(executor.createdJobs() == size_t(0));
    REQUIRE(executor.failedSteps() == size_t(0));
}

SECTION("testExecutorCancelled") {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;
    std::atomic<bool> cancelled{true};
    ctx.cancelled = &cancelled;

    FirmwareExecutor executor;
    FlashPlan plan;
    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isError());
}

SECTION("testExecutorFlashStep") {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    FirmwareExecutor executor;
    FlashPlan plan;
    FlashStep step;
    step.type = FlashStepType::FlashPartition;
    step.partitionName = "boot";
    step.data = ByteBuffer(4096, 0xAA);
    plan.steps.push_back(std::move(step));

    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());

    auto pipeResult = pipeline.run(ctx);
    REQUIRE(pipeResult.isOk());
    REQUIRE(pipeline.completedCount() == size_t(1));
}

SECTION("testExecutorGptStep") {
    VirtualJobDevice device;
    device.open();

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    FirmwareExecutor executor;
    FlashPlan plan;
    FlashStep step;
    step.type = FlashStepType::UpdateGPT;
    step.description = "Update GPT";
    plan.steps.push_back(std::move(step));

    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());

    auto pipeResult = pipeline.run(ctx);
    REQUIRE(pipeResult.isOk());
}

SECTION("testExecutorVerifyStep") {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    auto data = ByteBuffer(4096, 0xBB);
    device.writeToStorage(0, data);

    FirmwareExecutor executor;
    FlashPlan plan;
    FlashStep step;
    step.type = FlashStepType::VerifyPartition;
    step.partitionName = "boot";
    step.data = data;
    plan.steps.push_back(std::move(step));

    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());

    auto pipeResult = pipeline.run(ctx);
    REQUIRE(pipeResult.isOk());
}

SECTION("testExecutorEraseStep") {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    FirmwareExecutor executor;
    FlashPlan plan;
    FlashStep step;
    step.type = FlashStepType::ErasePartition;
    step.partitionName = "boot";
    plan.steps.push_back(std::move(step));

    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());

    auto pipeResult = pipeline.run(ctx);
    REQUIRE(pipeResult.isOk());
}

SECTION("testExecutorRebootStep") {
    VirtualJobDevice device;
    device.open();

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    FirmwareExecutor executor;
    FlashPlan plan;
    FlashStep step;
    step.type = FlashStepType::Reboot;
    step.description = "Reboot";
    plan.steps.push_back(std::move(step));

    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());
}

SECTION("testExecutorEmptyPlan") {
    VirtualJobDevice device;
    device.open();

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    FirmwareExecutor executor;
    FlashPlan plan;
    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.jobCount() == size_t(0));
}

SECTION("testExecutorProgressCallback") {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    job::JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    FirmwareExecutor executor;
    int cbCalls = 0;
    executor.setProgressCallback([&](const std::string&, float) { cbCalls++; });

    FlashPlan plan;
    FlashStep step;
    step.type = FlashStepType::FlashPartition;
    step.partitionName = "boot";
    step.data = ByteBuffer(4096, 0xCC);
    plan.steps.push_back(std::move(step));

    job::JobPipeline pipeline;
    auto result = executor.executePlan(plan, pipeline, ctx);
    REQUIRE(result.isOk());
    REQUIRE(cbCalls >= 1);
}

SECTION("testVirtualGeneratorGoodPackage") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->imageCount() >= 2);
    REQUIRE(pkg->metadata().vendor == std::string("QUALCOMM"));
}

SECTION("testVirtualGeneratorMissingImage") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateMissingImage("boot");
    REQUIRE(pkg != nullptr);
    REQUIRE(!pkg->hasImage("boot"));
}

SECTION("testVirtualGeneratorBadHash") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateBadHash();
    REQUIRE(pkg != nullptr);
    bool hasHash = false;
    for (const auto& img : pkg->images()) {
        if (!img.info.expectedHash.empty()) {
            hasHash = true;
            REQUIRE(img.info.expectedHash[0] == uint8_t(0xBB));
        }
    }
    REQUIRE(hasHash);
}

SECTION("testVirtualGeneratorCorruptManifest") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateCorruptManifest();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->metadata().vendor.empty());
}

SECTION("testVirtualGeneratorUnsupportedVendor") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateUnsupportedVendor();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->metadata().vendor == std::string("UNKNOWN_VENDOR"));
}

SECTION("testVirtualGeneratorDuplicatePartitions") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateDuplicatePartitions();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->imageCount() >= 1);
}

SECTION("testVirtualGeneratorVersionMismatch") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateVersionMismatch();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->metadata().version.major == uint32_t(0));
    REQUIRE(pkg->metadata().version.minor == uint32_t(0));
    REQUIRE(pkg->metadata().version.patch == uint32_t(1));
}

SECTION("testVirtualGeneratorMissingDependency") {
    VirtualPackageGenerator gen;
    auto pkg = gen.generateMissingDependency();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->dependencies().size() == size_t(1));
    REQUIRE(pkg->dependencies()[0].id == std::string("dep_firehose"));
}

SECTION("testVirtualReaderBasic") {
    VirtualPackageGenerator gen;
    VirtualPackageReader reader(gen);
    REQUIRE(reader.name() == std::string("VirtualReader"));
}

SECTION("testVirtualReaderNotFound") {
    VirtualPackageGenerator gen;
    VirtualPackageReader reader(gen);
    REQUIRE(!reader.canRead("nonexistent"));
    auto result = reader.read("nonexistent");
    REQUIRE(result.isError());
}

SECTION("testVirtualReaderSetPackage") {
    VirtualPackageGenerator gen;
    VirtualPackageReader reader(gen);
    auto pkg = gen.generateGoodPackage();
    reader.setPackage("test_key", std::move(pkg));
    REQUIRE(reader.canRead("test_key"));
}

SECTION("testVirtualRepositoryGood") {
    VirtualFirmwareRepository repo;
    auto pkg = repo.getGoodPackage();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->metadata().vendor == std::string("QUALCOMM"));
}

SECTION("testVirtualRepositoryCorrupt") {
    VirtualFirmwareRepository repo;
    auto pkg = repo.getCorruptPackage();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->metadata().vendor.empty());
}

SECTION("testVirtualRepositoryUnsupported") {
    VirtualFirmwareRepository repo;
    auto pkg = repo.getUnsupportedPackage();
    REQUIRE(pkg != nullptr);
    REQUIRE(pkg->metadata().vendor == std::string("UNKNOWN_VENDOR"));
}

SECTION("testEmptyManifest") {
    PackageManifest manifest;
    REQUIRE(manifest.images.empty());
    REQUIRE(manifest.dependencies.empty());
}

SECTION("testPackageWithNoImages") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    REQUIRE(pkg.imageCount() == size_t(0));
    auto result = pkg.validateBasic();
    REQUIRE(result.valid);
    REQUIRE(!result.warnings.empty());
}

SECTION("testImageWithEmptyName") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    FirmwareImage img;
    pkg.addImage(std::move(img));
    auto result = pkg.validateBasic();
    REQUIRE(!result.valid);
}

SECTION("testMultipleImagesSamePartitionFails") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    
    FirmwareImage img1;
    img1.info.name = "boot";
    img1.info.partitionName = "boot_a";
    REQUIRE(pkg.addImage(std::move(img1)).isOk());

    FirmwareImage img2;
    img2.info.name = "boot_backup";
    img2.info.partitionName = "boot_a";
    REQUIRE(pkg.addImage(std::move(img2)).isOk());
    FirmwareValidator validator;
    auto vResult = validator.validate(pkg);
    REQUIRE(!vResult.valid);
}

SECTION("testLargeImageData") {
    auto pkg = FirmwarePackage(PackageMetadata{});
    FirmwareImage img;
    img.info.name = "large";
    img.data = ByteBuffer(1024 * 1024, 0xFF);
    img.dataLoaded = true;
    auto result = pkg.addImage(std::move(img));
    REQUIRE(result.isOk());
    REQUIRE(pkg.imageCount() == size_t(1));
}

SECTION("testMetadataRoundTrip") {
    PackageMetadata meta;
    meta.vendor = "MEDIATEK";
    meta.platform = "MT6789";
    meta.chipset = "MT6789";
    meta.protocol = "sahara";
    meta.mode = "emmc";
    meta.version = FirmwareVersion{2, 1, 0};
    meta.buildDate = "2026-06-30";
    meta.author = "Test";
    meta.packageUuid = "test123";
    meta.packageSize = 65536;
    meta.description = "Test package";

    auto pkg = FirmwarePackage(std::move(meta));
    REQUIRE(pkg.metadata().vendor == std::string("MEDIATEK"));
    REQUIRE(pkg.metadata().platform == std::string("MT6789"));
    REQUIRE(pkg.metadata().version.toString() == std::string("2.1.0"));
    REQUIRE(pkg.metadata().packageUuid == std::string("test123"));
}

SECTION("testValidatorNoErrorsOnEmpty") {
    PackageMetadata meta;
    meta.vendor = "QUALCOMM";
    meta.platform = "SM8450";
    auto pkg = FirmwarePackage(std::move(meta));
    FirmwareValidator validator;
    auto result = validator.validate(pkg);
    REQUIRE(result.valid);
    REQUIRE(result.errors.empty());
}

}
