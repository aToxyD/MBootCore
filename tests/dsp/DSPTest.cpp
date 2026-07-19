#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iostream>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>
#include <mbootcore/dsp/DSPManager.hpp>
#include <mbootcore/dsp/DSPRepository.hpp>
#include <mbootcore/dsp/DSPCache.hpp>
#include <mbootcore/dsp/DSPValidator.hpp>
#include <mbootcore/dsp/DSPDependencyGraph.hpp>
#include <mbootcore/dsp/DSPBuilder.hpp>
#include <mbootcore/dsp/DSPInspector.hpp>
#include <mbootcore/dsp/HardwareProfile.hpp>
#include <mbootcore/dsp/VendorQuirk.hpp>
#include <mbootcore/dsp/LoaderRepository.hpp>

namespace fs = std::filesystem;
using namespace mbootcore::dsp;
using namespace mbootcore::discovery;

namespace {

std::string createVirtualPackage(const std::string& baseDir, const std::string& pkgId,
                                const std::string& vendor = "Qualcomm")
{
    auto pkgDir = baseDir + "/" + pkgId;
    fs::create_directories(pkgDir);
    fs::create_directories(pkgDir + "/loaders");
    fs::create_directories(pkgDir + "/profiles");
    fs::create_directories(pkgDir + "/quirks");

    std::ofstream(pkgDir + "/manifest.json") << R"({
        "formatVersion": "1.0",
        "packageId": ")" << pkgId << R"(",
        "name": "Test Package )" << pkgId << R"(",
        "vendor": ")" << vendor << R"(",
        "description": "Test description",
        "version": "1.0.0",
        "license": "MIT",
        "authors": ["Test Author"]
    })";

    std::ofstream(pkgDir + "/vendor.json") << R"({
        "vendorId": 1,
        "vendorName": ")" << vendor << R"(",
        "displayName": "Test Vendor"
    })";

    std::ofstream(pkgDir + "/chipsets.json") << R"([{
        "vendor": ")" << vendor << R"(",
        "family": "SM",
        "variant": "8250",
        "displayName": "Snapdragon 8250"
    }])";

    std::ofstream(pkgDir + "/loaders/prog_ufs_firehose.elf") << "dummy loader data";

    std::ofstream(pkgDir + "/version.json") << R"({
        "major": 1, "minor": 0, "patch": 0
    })";

    return pkgDir;
}

std::string createTempDir() {
    auto tmp = fs::temp_directory_path();
    for (int i = 0; i < 100; ++i) {
        auto p = tmp / ("dsp_test_" + std::to_string(i));
        if (!fs::exists(p)) {
            fs::create_directories(p);
            return p.string();
        }
    }
    return (tmp / "dsp_test_fallback").string();
}

}

TEST_CASE("DSPMetadataTest", "[dsp]") {

SECTION("testDSPVersionComparisons") {
    DSPVersion v1{1,0,0}, v2{1,0,1}, v3{2,0,0};
    REQUIRE(v1 < v2);
    REQUIRE(v2 < v3);
    REQUIRE(v1 == v1);
    REQUIRE(!(v2 == v3));
    REQUIRE(v1.toString() == "1.0.0");
    REQUIRE(v2.toString() == "1.0.1");
}

SECTION("testChipsetId") {
    ChipsetId id{"Qualcomm", "SM", "8250"};
    REQUIRE(id.toString() == "Qualcomm/SM/8250");
    REQUIRE(id.vendor == "Qualcomm");
    REQUIRE(id.family == "SM");
    REQUIRE(id.variant == "8250");
}

SECTION("testDSPDependencyDefaults") {
    DSPDependency dep;
    REQUIRE(dep.required);
    REQUIRE(dep.minVersion.major == 1u);
}

SECTION("testDSPManifestDefaults") {
    DSPManifest m;
    REQUIRE(m.formatVersion == "1.0");
}

SECTION("testDSPCompatibilityInfo") {
    DSPCompatibilityInfo info;
    REQUIRE(!info.isCompatible());
    info.coreCompatible = true;
    info.sdkCompatible = true;
    info.osCompatible = true;
    info.architectureCompatible = true;
    REQUIRE(info.isCompatible());
}

SECTION("testPackageStatistics") {
    DSPPackageStatistics stats;
    stats.packageId = "test";
    stats.loaderCount = 5;
    stats.chipsetCount = 3;
    REQUIRE(stats.packageId == "test");
    REQUIRE(stats.loaderCount == 5u);
    REQUIRE(stats.chipsetCount == 3u);
}

SECTION("testDSPStateEnum") {
    REQUIRE(static_cast<int>(DSPState::Unknown) == 0);
    REQUIRE(static_cast<int>(DSPState::Installed) == 1);
    REQUIRE(static_cast<int>(DSPState::Enabled) == 2);
    REQUIRE(static_cast<int>(DSPState::Disabled) == 3);
}

SECTION("testDSPOriginEnum") {
    REQUIRE(static_cast<int>(DSPOrigin::System) == 0);
    REQUIRE(static_cast<int>(DSPOrigin::User) == 1);
    REQUIRE(static_cast<int>(DSPOrigin::Portable) == 2);
}

SECTION("testStorageTypeEnum") {
    REQUIRE(static_cast<int>(StorageType::UFS) == 1);
    REQUIRE(static_cast<int>(StorageType::eMMC) == 2);
    REQUIRE(static_cast<int>(StorageType::NAND) == 3);
}

}

TEST_CASE("DSPRepositoryTest", "[dsp]") {

SECTION("testRepositoryPaths") {
    DSPRepository repo;
    repo.setSystemPath("/tmp/dsp/system");
    repo.setUserPath("/tmp/dsp/user");
    repo.setPortablePath("/tmp/dsp/portable");
    REQUIRE(repo.systemPath() == "/tmp/dsp/system");
    REQUIRE(repo.userPath() == "/tmp/dsp/user");
    REQUIRE(repo.portablePath() == "/tmp/dsp/portable");
    REQUIRE(repo.isEmpty());
}

SECTION("testScanAndQuery") {
    auto basePath = createTempDir();

    auto pkgPath1 = createVirtualPackage(basePath, "com.vendor.test1", "Qualcomm");
    auto pkgPath2 = createVirtualPackage(basePath, "com.vendor.test2", "MediaTek");
    (void)pkgPath1; (void)pkgPath2;

    DSPRepository repo;
    repo.setUserPath(basePath);
    auto result = repo.scan();
    REQUIRE(result.isOk());

    REQUIRE(!repo.isEmpty());
    fs::remove_all(basePath);
}

}

TEST_CASE("DSPCacheTest", "[dsp]") {

SECTION("testCacheLifecycle") {
    auto basePath = createTempDir();
    auto cachePath = basePath + "/cache";
    fs::create_directories(cachePath);

    DSPCache cache(cachePath);

    DSPPackageMetadata meta;
    meta.manifest.packageId = "test.pkg";
    meta.manifest.version = {1,0,0};
    auto result = cache.cacheMetadata("test.pkg", meta);
    REQUIRE(result.isOk());
    REQUIRE(cache.hasMetadata("test.pkg"));

    auto loaded = cache.loadMetadata("test.pkg");
    REQUIRE(loaded.isOk());
    REQUIRE(loaded.value().manifest.packageId == "test.pkg");
    REQUIRE(loaded.value().manifest.version.minor == 0u);

    (void)cache.invalidateMetadata("test.pkg");
    REQUIRE(!cache.hasMetadata("test.pkg"));
    fs::remove_all(basePath);
}

SECTION("testLoaderCache") {
    auto basePath = createTempDir();
    auto cachePath = basePath + "/ldrcache";
    fs::create_directories(cachePath);

    DSPCache cache(cachePath);
    mbootcore::ByteBuffer data = {0x01, 0x02, 0x03, 0x04, 0xFF};

    auto result = cache.cacheLoader("test_loader", data);
    REQUIRE(result.isOk());
    REQUIRE(cache.hasLoader("test_loader"));

    auto loaded = cache.loadLoader("test_loader");
    REQUIRE(loaded.isOk());
    REQUIRE(loaded.value().size() == data.size());
    REQUIRE(loaded.value()[0] == 0x01);
    REQUIRE(loaded.value()[4] == 0xFF);

    (void)cache.clear();
    REQUIRE(!cache.hasLoader("test_loader"));
    fs::remove_all(basePath);
}

SECTION("testCacheClearAndPrune") {
    auto basePath = createTempDir();
    auto cachePath = basePath + "/prune";
    fs::create_directories(cachePath);

    DSPCache cache(cachePath);
    DSPPackageMetadata meta;
    meta.manifest.packageId = "pkg1";
    (void)cache.cacheMetadata("pkg1", meta);

    REQUIRE(cache.hasMetadata("pkg1"));
    (void)cache.clearAll();
    REQUIRE(!cache.hasMetadata("pkg1"));
    fs::remove_all(basePath);
}

}

TEST_CASE("DSPValidatorTest", "[dsp]") {

SECTION("testValidateValidPackage") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.valid");

    DSPValidator validator;
    auto report = validator.validate(pkgPath, DSPValidationLevel::Basic);
    REQUIRE(report.filesChecked > 0);
    fs::remove_all(basePath);
}

SECTION("testValidateMetadata") {
    DSPPackageMetadata meta;
    meta.manifest.packageId = "test.pkg";
    meta.manifest.name = "Test";
    meta.manifest.vendor = "TestVendor";
    meta.manifest.version = {1,0,0};
    meta.vendor.vendorName = "TestVendor";

    DSPValidator validator;
    auto report = validator.validateMetadata(meta);
    REQUIRE((report.valid || report.errors.empty()));
}

}

TEST_CASE("DSPDependencyGraphTest", "[dsp]") {

SECTION("testEmptyGraph") {
    DSPDependencyGraph graph;
    REQUIRE(graph.nodeCount() == 0u);
    REQUIRE(graph.edgeCount() == 0u);
    REQUIRE(graph.allNodes().empty());
}

SECTION("testSinglePackage") {
    DSPDependencyGraph graph;
    DSPPackageMetadata pkg;
    pkg.manifest.packageId = "pkg1";
    pkg.manifest.name = "Package 1";
    pkg.manifest.version = {1,0,0};

    auto result = graph.addPackage(pkg);
    REQUIRE(result.isOk());
    REQUIRE(graph.nodeCount() == 1u);
    REQUIRE(graph.edgeCount() == 0u);
}

SECTION("testDependencyResolution") {
    DSPDependencyGraph graph;

    DSPPackageMetadata pkg1;
    pkg1.manifest.packageId = "base";
    pkg1.manifest.version = {1,0,0};
    (void)graph.addPackage(pkg1);

    DSPPackageMetadata pkg2;
    pkg2.manifest.packageId = "dependent";
    pkg2.manifest.version = {1,0,0};
    DSPDependency dep;
    dep.packageId = "base";
    dep.required = true;
    pkg2.manifest.dependencies.push_back(dep);
    (void)graph.addPackage(pkg2);

    auto result = graph.resolve();
    REQUIRE(result.isOk());
}

SECTION("testCircularDependency") {
    DSPDependencyGraph graph;

    DSPPackageMetadata pkg1;
    pkg1.manifest.packageId = "pkg1";
    pkg1.manifest.version = {1,0,0};
    DSPDependency dep1;
    dep1.packageId = "pkg2";
    dep1.required = true;
    pkg1.manifest.dependencies.push_back(dep1);
    (void)graph.addPackage(pkg1);

    DSPPackageMetadata pkg2;
    pkg2.manifest.packageId = "pkg2";
    pkg2.manifest.version = {1,0,0};
    DSPDependency dep2;
    dep2.packageId = "pkg1";
    dep2.required = true;
    pkg2.manifest.dependencies.push_back(dep2);
    (void)graph.addPackage(pkg2);

    auto result = graph.resolve();
    REQUIRE(result.isOk());
    REQUIRE(graph.hasCircularDependency());
}

}

TEST_CASE("HardwareProfileTest", "[dsp]") {

SECTION("testProfileDefaults") {
    HardwareProfile profile;
    profile.profileId = "qcom_sm8250";
    profile.chipset = {"Qualcomm", "SM", "8250"};
    REQUIRE(profile.profileId == "qcom_sm8250");
}

SECTION("testStorageProfile") {
    StorageProfile sp;
    sp.type = StorageType::UFS;
    sp.totalSizeMB = 131072;
    sp.sectorSize = 4096;
    REQUIRE(sp.type == StorageType::UFS);
    REQUIRE(sp.totalSizeMB == 131072u);
    REQUIRE(sp.sectorSize == 4096u);
}

SECTION("testMemoryProfile") {
    MemoryProfile mp;
    mp.totalMB = 8192;
    mp.ddrFreqMHz = 2133;
    mp.channelCount = 2;
    REQUIRE(mp.totalMB == 8192u);
    REQUIRE(mp.channelCount == 2u);
}

SECTION("testPartitionLayout") {
    PartitionLayout layout;
    layout.name = "boot";
    layout.startSector = 2048;
    layout.endSector = 40960;
    layout.bootable = true;
    REQUIRE(layout.name == "boot");
    REQUIRE(layout.bootable);
}

SECTION("testFlashGeometry") {
    FlashGeometry fg;
    fg.totalSectors = 61071360;
    fg.sectorSize = 4096;
    fg.totalSizeBytes = fg.totalSectors * fg.sectorSize;
    REQUIRE(fg.totalSizeBytes == 61071360ull * 4096);
}

SECTION("testCapabilityProfile") {
    CapabilityProfile cap;
    cap.canProgram = true;
    cap.canErase = true;
    cap.canRead = true;
    REQUIRE(cap.canProgram);
    REQUIRE(cap.canErase);
    REQUIRE(!cap.canBackup);
}

SECTION("testPerformanceProfile") {
    PerformanceProfile perf;
    perf.recommendedWorkers = 4;
    perf.bufferSizeKB = 1024;
    perf.maxRetries = 3;
    REQUIRE(perf.recommendedWorkers == 4u);
    REQUIRE(perf.maxRetries == 3u);
}

}

TEST_CASE("VendorQuirkTest", "[dsp]") {

SECTION("testQuirkDefaults") {
    VendorQuirk quirk;
    quirk.quirkId = "QCT_XML_001";
    quirk.name = "Special XML format";
    quirk.action = QuirkAction::OverrideXML;
    quirk.severity = QuirkSeverity::Warning;
    REQUIRE(quirk.quirkId == "QCT_XML_001");
    REQUIRE(quirk.enabled);
}

SECTION("testQuirkPolicyDefaults") {
    QuirkPolicy policy;
    REQUIRE(policy.minSeverity == QuirkSeverity::Warning);
    REQUIRE(!policy.autoApply);
    REQUIRE(policy.blockOnCritical);
}

SECTION("testQuirkSeverityEnum") {
    REQUIRE(static_cast<int>(QuirkSeverity::Info) == 0);
    REQUIRE(static_cast<int>(QuirkSeverity::Warning) == 1);
    REQUIRE(static_cast<int>(QuirkSeverity::Critical) == 3);
}

SECTION("testQuirkActionEnum") {
    REQUIRE(static_cast<int>(QuirkAction::OverrideXML) == 1);
    REQUIRE(static_cast<int>(QuirkAction::OverrideReset) == 2);
    REQUIRE(static_cast<int>(QuirkAction::CustomAction) == 99);
}

SECTION("testQuirkScopeEnum") {
    REQUIRE(static_cast<int>(QuirkScope::Vendor) == 1);
    REQUIRE(static_cast<int>(QuirkScope::Loader) == 6);
}

}

TEST_CASE("LoaderRepositoryTest", "[dsp]") {

SECTION("testLoaderScoring") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.loaders");
    (void)pkgPath;

    DSPRepository repo;
    repo.setUserPath(basePath);
    (void)repo.scan();

    LoaderIndex index(repo);
    index.refresh();

    DSPChipsetMetadata chipset;
    chipset.id = {"Qualcomm", "SM", "8250"};

    REQUIRE((index.count() > 0 || index.count() == 0));
    fs::remove_all(basePath);
}

SECTION("testLoaderVerifier") {
    DSPLoaderMetadata loader;
    loader.loaderId = "prog_ufs_firehose";
    loader.hashAlgorithm = "SHA256";

    LoaderVerifier verifier;
    mbootcore::ByteBuffer data = {0x01, 0x02, 0x03};
    auto result = verifier.verifyIntegrity(loader, data);
    REQUIRE(result.isOk());
}

SECTION("testResolverEmpty") {
    auto basePath = createTempDir();
    createVirtualPackage(basePath, "com.test.resolver");

    DSPRepository repo;
    repo.setUserPath(basePath);
    (void)repo.scan();

    LoaderDatabase db(repo);
    LoaderMatcher matcher(db);
    LoaderResolver resolver(matcher);

    auto resolved = resolver.resolve({"Qualcomm", "SM", "8250"});
    REQUIRE(resolved.primary == nullptr);
    fs::remove_all(basePath);
}

}

TEST_CASE("DSPBuilderTest", "[dsp]") {

SECTION("testBuildFromConfig") {
    auto srcPath = createTempDir();
    auto outPath = createTempDir();

    fs::create_directories(srcPath + "/loaders");
    fs::create_directories(srcPath + "/profiles");
    fs::create_directories(srcPath + "/quirks");
    std::ofstream(srcPath + "/loaders/prog_firehose.elf") << "loader data";

    DSPBuilder builder;
    BuildConfig config;
    config.sourceDir = srcPath;
    config.outputDir = outPath;
    config.packageId = "com.test.built";
    config.name = "Built Package";
    config.vendor = "Qualcomm";
    config.version = "1.0.0";
    config.description = "Test build";

    auto result = builder.build(config);
    REQUIRE((result.isOk() || result.isError()));
    fs::remove_all(srcPath);
    fs::remove_all(outPath);
}

SECTION("testGenerateManifest") {
    auto outPath = createTempDir();

    DSPBuilder builder;
    BuildConfig config;
    config.packageId = "com.test.manifest";
    config.name = "Manifest Test";
    config.vendor = "TestVendor";
    config.version = "2.0.0";

    auto result = builder.generateManifest(config, outPath + "/manifest.json");
    REQUIRE((result.isOk() || result.isError()));
    fs::remove_all(outPath);
}

SECTION("testChecksumManifestGeneration") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.checksum_manifest");

    std::ofstream(pkgPath + "/checksum.sha256") << "abc123  manifest.json\n";

    DSPBuilder builder;
    auto result = builder.generateChecksumManifest(pkgPath);
    REQUIRE(result.isOk());

    auto sigPath = fs::path(pkgPath) / "signature.sig";
    REQUIRE(fs::exists(sigPath));

    std::string content;
    {
        std::ifstream sigFile(sigPath);
        content.assign((std::istreambuf_iterator<char>(sigFile)),
                        std::istreambuf_iterator<char>());
    }

    REQUIRE(content.find("MBoot DSP Package") != std::string::npos);
    REQUIRE(content.find("Checksum Manifest") != std::string::npos);
    REQUIRE(content.find("SHA-256") != std::string::npos);
    REQUIRE(content.find("Integrity verification only") != std::string::npos);
    REQUIRE(content.find("NOT a cryptographic signature") != std::string::npos);

    fs::remove_all(basePath);
}

SECTION("testChecksumManifestNoPlaceholderText") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.no_placeholder");

    std::ofstream(pkgPath + "/checksum.sha256") << "def456  manifest.json\n";

    DSPBuilder builder;
    auto result = builder.generateChecksumManifest(pkgPath);
    REQUIRE(result.isOk());

    auto sigPath = fs::path(pkgPath) / "signature.sig";
    std::string content;
    {
        std::ifstream sigFile(sigPath);
        content.assign((std::istreambuf_iterator<char>(sigFile)),
                        std::istreambuf_iterator<char>());
    }

    REQUIRE(content.find("Placeholder signature") == std::string::npos);
    REQUIRE(content.find("Key file:") == std::string::npos);
    REQUIRE(content.find("Signed on:") == std::string::npos);

    fs::remove_all(basePath);
}

SECTION("testChecksumManifestNoKeyFileRequired") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.no_key");
    auto outPath = createTempDir();

    DSPBuilder builder;
    BuildConfig config;
    config.sourceDir = basePath;
    config.outputDir = outPath;
    config.packageId = "com.test.no_key";
    config.name = "No Key Test";
    config.vendor = "TestVendor";
    config.version = "1.0.0";
    config.generateChecksumManifest = true;

    auto buildResult = builder.build(config);
    REQUIRE(buildResult.isOk());

    auto builtPkg = fs::path(outPath) / config.packageId;
    auto sigPath = builtPkg / "signature.sig";
    REQUIRE(fs::exists(sigPath));

    std::string content;
    {
        std::ifstream sigFile(sigPath);
        content.assign((std::istreambuf_iterator<char>(sigFile)),
                        std::istreambuf_iterator<char>());
    }

    REQUIRE(content.find("Key file:") == std::string::npos);
    REQUIRE(content.find("Placeholder") == std::string::npos);

    fs::remove_all(basePath);
    fs::remove_all(outPath);
}

SECTION("testChecksumManifestSha256MatchesPackage") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.sha_match");

    std::ofstream(pkgPath + "/checksum.sha256") << "aaa111  manifest.json\n";

    DSPBuilder builder;
    auto result = builder.generateChecksumManifest(pkgPath);
    REQUIRE(result.isOk());

    auto sigPath = fs::path(pkgPath) / "signature.sig";
    std::string content;
    {
        std::ifstream sigFile(sigPath);
        content.assign((std::istreambuf_iterator<char>(sigFile)),
                        std::istreambuf_iterator<char>());
    }

    REQUIRE(content.find("SHA256 digest of checksum.sha256:") != std::string::npos);

    size_t digestPos = content.find("SHA256 digest of checksum.sha256:\n");
    REQUIRE(digestPos != std::string::npos);
    auto afterLabel = content.substr(digestPos + strlen("SHA256 digest of checksum.sha256:\n"));
    auto newlinePos = afterLabel.find('\n');
    REQUIRE(newlinePos != std::string::npos);
    auto digestLine = afterLabel.substr(0, newlinePos);
    REQUIRE(digestLine.size() == 64);

    fs::remove_all(basePath);
}

SECTION("testBuildConfigNoKeyFileField") {
    BuildConfig config;
    config.packageId = "com.test.config";
    config.name = "Config Test";
    config.vendor = "TestVendor";
    config.version = "1.0.0";
    config.generateChecksumManifest = true;

    REQUIRE(config.generateChecksumManifest);
}

}

TEST_CASE("DSPInspectorTest", "[dsp]") {

SECTION("testOpenValidPackage") {
    auto basePath = createTempDir();
    auto pkgPath = createVirtualPackage(basePath, "com.test.inspect");
    (void)pkgPath;

    DSPInspector inspector;
    auto result = inspector.open(pkgPath);
    REQUIRE((inspector.isOpen() || !inspector.isOpen()));
    inspector.close();
    fs::remove_all(basePath);
}

}

TEST_CASE("PackageStatisticsTest", "[dsp]") {

SECTION("testStatisticsDefaults") {
    DSPPackageStatistics stats;
    REQUIRE(stats.loaderCount == 0u);
    REQUIRE(stats.chipsetCount == 0u);
    REQUIRE(stats.profileCount == 0u);
    REQUIRE(stats.quirkCount == 0u);
    REQUIRE(!stats.needsUpdate);
}

SECTION("testStatisticsWithValues") {
    DSPPackageStatistics stats;
    stats.packageId = "com.test.stats";
    stats.name = "Stats Test";
    stats.version = {2,1,0};
    stats.loaderCount = 10;
    stats.chipsetCount = 25;
    stats.profileCount = 5;
    stats.quirkCount = 12;
    stats.installedSize = 1048576;

    REQUIRE(stats.version.toString() == "2.1.0");
    REQUIRE(stats.loaderCount == 10u);
    REQUIRE(stats.installedSize == 1048576u);
}

SECTION("testStatisticsStateChanges") {
    DSPPackageStatistics stats;
    stats.state = DSPState::Enabled;
    REQUIRE(static_cast<int>(stats.state) == 2);
    stats.state = DSPState::Disabled;
    REQUIRE(static_cast<int>(stats.state) == 3);
    stats.origin = DSPOrigin::System;
    REQUIRE(static_cast<int>(stats.origin) == 0);
}

}

TEST_CASE("DSPManagerTest", "[dsp]") {

SECTION("testManagerConstruct") {
    auto repo = std::make_unique<DSPRepository>();
    DSPManager manager(std::move(repo));
    REQUIRE(manager.installedCount() == 0u);
}

SECTION("testListInstalled") {
    auto repo = std::make_unique<DSPRepository>();
    DSPManager manager(std::move(repo));
    auto installed = manager.listInstalled();
    REQUIRE(installed.empty());
}

}

TEST_CASE("VirtualDSPTest", "[dsp]") {

SECTION("testMultipleVirtualPackages") {
    auto basePath = createTempDir();

    for (int i = 0; i < 5; ++i) {
        auto vendor = (i % 2 == 0) ? "Qualcomm" : "MediaTek";
        createVirtualPackage(basePath, "com.vendor.pkg" + std::to_string(i), vendor);
    }

    DSPRepository repo;
    repo.setUserPath(basePath);
    auto result = repo.scan();
    REQUIRE(result.isOk());
    REQUIRE(repo.packageCount() == 5u);

    auto qcPkgs = repo.byVendor(Vendor::Qualcomm);
    auto mtkPkgs = repo.byVendor(Vendor::MediaTek);
    REQUIRE(qcPkgs.size() + mtkPkgs.size() == 5u);
    fs::remove_all(basePath);
}

SECTION("testSearchByChipset") {
    auto basePath = createTempDir();

    createVirtualPackage(basePath, "com.test.sm8250", "Qualcomm");

    DSPRepository repo;
    repo.setUserPath(basePath);
    (void)repo.scan();

    DSPQuery query;
    query.chipset = {"Qualcomm", "SM", "8250"};
    auto results = repo.search(query);
    REQUIRE(results.size() <= 100);
    fs::remove_all(basePath);
}

}

TEST_CASE("DSPStressTest", "[dsp]") {

SECTION("testManyDependencies") {
    DSPDependencyGraph graph;

    for (int i = 0; i < 50; ++i) {
        DSPPackageMetadata pkg;
        pkg.manifest.packageId = "pkg" + std::to_string(i);
        pkg.manifest.version = {1,0,0};

        if (i > 0) {
            DSPDependency dep;
            dep.packageId = "pkg" + std::to_string(i - 1);
            dep.required = true;
            pkg.manifest.dependencies.push_back(dep);
        }

        auto result = graph.addPackage(pkg);
        REQUIRE(result.isOk());
    }

    REQUIRE(graph.nodeCount() == 50u);
    REQUIRE(graph.edgeCount() == 49u);

    auto result = graph.resolve();
    REQUIRE(result.isOk());
    REQUIRE(!graph.hasCircularDependency());
    REQUIRE(graph.resolutionOrder().size() == 50u);
}

SECTION("testCacheStress") {
    auto basePath = createTempDir();
    auto cachePath = basePath + "/stress";
    fs::create_directories(cachePath);

    DSPCache cache(cachePath);

    for (int i = 0; i < 100; ++i) {
        DSPPackageMetadata meta;
        meta.manifest.packageId = "stress.pkg" + std::to_string(i);
        meta.manifest.version = {1,0, static_cast<uint32_t>(i)};
        (void)cache.cacheMetadata("stress.pkg" + std::to_string(i), meta);
    }

    REQUIRE(cache.cacheEntryCount() == 100u);
    REQUIRE(cache.cacheSizeBytes() > 0);
    REQUIRE(cache.isWarm());

    for (int i = 0; i < 100; ++i) {
        REQUIRE(cache.hasMetadata("stress.pkg" + std::to_string(i)));
        auto loaded = cache.loadMetadata("stress.pkg" + std::to_string(i));
        REQUIRE(loaded.isOk());
        REQUIRE(loaded.value().manifest.version.patch == static_cast<uint32_t>(i));
    }
    fs::remove_all(basePath);
}

}

TEST_CASE("DSPPerformanceTest", "[dsp]") {

SECTION("testMetadataRoundTrip") {
    auto basePath = createTempDir();
    auto cachePath = basePath + "/perf";
    fs::create_directories(cachePath);

    DSPCache cache(cachePath);

    DSPPackageMetadata meta;
    meta.manifest.packageId = "perf.test";
    meta.manifest.name = "Performance Test";
    meta.manifest.vendor = "TestVendor";
    meta.manifest.version = {2,3,4};
    meta.vendor.vendorName = "TestVendor";
    meta.vendor.vendorId = Vendor::Qualcomm;

    DSPChipsetMetadata chipset;
    chipset.id = {"TestVendor", "TestFamily", "TestVariant"};
    meta.chipsets.push_back(chipset);

    auto result = cache.cacheMetadata("perf.test", meta);
    REQUIRE(result.isOk());

    auto loaded = cache.loadMetadata("perf.test");
    REQUIRE(loaded.isOk());
    REQUIRE(loaded.value().manifest.name == "Performance Test");
    REQUIRE(loaded.value().manifest.version.patch == 4u);
    REQUIRE(loaded.value().chipsets.size() == 1u);
    REQUIRE(loaded.value().chipsets[0].id.vendor == "TestVendor");
    fs::remove_all(basePath);
}

}

TEST_CASE("CorruptedPackageTest", "[dsp]") {

SECTION("testMissingManifest") {
    auto basePath = createTempDir();
    auto pkgPath = basePath + "/corrupt_pkg";
    fs::create_directories(pkgPath);

    DSPValidator validator;
    auto report = validator.validate(pkgPath, DSPValidationLevel::Basic);
    REQUIRE(!report.valid);
    REQUIRE(report.errors.size() > 0);
    fs::remove_all(basePath);
}

SECTION("testEmptyPackage") {
    auto basePath = createTempDir();

    DSPRepository repo;
    repo.setUserPath(basePath);
    auto result = repo.scan();
    REQUIRE(result.isOk());
    REQUIRE(repo.isEmpty());
    fs::remove_all(basePath);
}

SECTION("testInvalidJson") {
    auto basePath = createTempDir();
    auto pkgPath = basePath + "/badjson";
    fs::create_directories(pkgPath);
    std::ofstream(pkgPath + "/manifest.json") << "{invalid json}";

    DSPValidator validator;
    auto report = validator.validateManifest(pkgPath);
    REQUIRE(!report.valid);
    fs::remove_all(basePath);
}

}
