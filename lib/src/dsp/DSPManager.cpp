#include <mbootcore/dsp/DSPManager.hpp>
#include <mbootcore/dsp/DSPRepository.hpp>
#include <mbootcore/dsp/DSPValidator.hpp>
#include <mbootcore/dsp/DSPCache.hpp>
#include <mbootcore/dsp/DSPInspector.hpp>

#include "SafeParser.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cstring>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <ctime>

namespace fs = std::filesystem;

namespace mbootcore {
namespace dsp {

namespace {

std::string readFileContents(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    auto size = file.tellg();
    if (size <= 0) return {};
    file.seekg(0);
    std::string content(static_cast<size_t>(size), '\0');
    file.read(content.data(), size);
    return content;
}






discovery::Vendor stringToVendor(const std::string& s) {
    if (s == "Qualcomm")   return discovery::Vendor::Qualcomm;
    if (s == "MediaTek")   return discovery::Vendor::MediaTek;
    if (s == "UNISOC")     return discovery::Vendor::UNISOC;
    if (s == "Samsung")    return discovery::Vendor::Samsung;
    if (s == "Rockchip")   return discovery::Vendor::Rockchip;
    if (s == "Spreadtrum") return discovery::Vendor::Spreadtrum;
    if (s == "Apple")      return discovery::Vendor::Apple;
    if (s == "Google")     return discovery::Vendor::Google;
    if (s == "Huawei")     return discovery::Vendor::Huawei;
    if (s == "Custom")     return discovery::Vendor::Custom;
    return discovery::Vendor::Unknown;
}



discovery::ProtocolType stringToProtocol(const std::string& s) {
    if (s == "Sahara")        return discovery::ProtocolType::Sahara;
    if (s == "Firehose")      return discovery::ProtocolType::Firehose;
    if (s == "Fastboot")      return discovery::ProtocolType::Fastboot;
    if (s == "MediaTekBROM")  return discovery::ProtocolType::MediaTekBROM;
    if (s == "MediaTekDA")    return discovery::ProtocolType::MediaTekDA;
    if (s == "UNISOCBootROM") return discovery::ProtocolType::UNISOCBootROM;
    if (s == "UNISOCFDL")     return discovery::ProtocolType::UNISOCFDL;
    if (s == "USBStream")     return discovery::ProtocolType::USBStream;
    if (s == "Custom")        return discovery::ProtocolType::Custom;
    return discovery::ProtocolType::Unknown;
}



discovery::BootMode stringToBootMode(const std::string& s) {
    if (s == "BootROM")      return discovery::BootMode::BootROM;
    if (s == "EDL")          return discovery::BootMode::EDL;
    if (s == "Firehose")     return discovery::BootMode::Firehose;
    if (s == "Fastboot")     return discovery::BootMode::Fastboot;
    if (s == "ADB")          return discovery::BootMode::ADB;
    if (s == "Recovery")     return discovery::BootMode::Recovery;
    if (s == "DownloadMode") return discovery::BootMode::DownloadMode;
    if (s == "Preloader")    return discovery::BootMode::Preloader;
    if (s == "BROM")         return discovery::BootMode::BROM;
    if (s == "Custom")       return discovery::BootMode::Custom;
    return discovery::BootMode::Unknown;
}



StorageType stringToStorageType(const std::string& s) {
    if (s == "UFS")  return StorageType::UFS;
    if (s == "eMMC") return StorageType::eMMC;
    if (s == "NAND") return StorageType::NAND;
    if (s == "NOR")  return StorageType::NOR;
    if (s == "SPI")  return StorageType::SPI;
    if (s == "SD")   return StorageType::SD;
    if (s == "NVMe") return StorageType::NVMe;
    return StorageType::Unknown;
}

std::string dspStateToString(DSPState s) {
    switch (s) {
        case DSPState::Installed:    return "installed";
        case DSPState::Enabled:      return "enabled";
        case DSPState::Disabled:     return "disabled";
        case DSPState::Corrupted:    return "corrupted";
        case DSPState::Incompatible: return "incompatible";
        default: return "unknown";
    }
}



std::chrono::system_clock::time_point stringToTimePoint(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        ss.clear();
        ss.str(s);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    auto tt = std::mktime(&tm);
    if (tt < 0) tt = 0;
    return std::chrono::system_clock::from_time_t(tt);
}



} // anonymous namespace

DSPManager::DSPManager(std::unique_ptr<DSPRepository> repository)
    : m_repository(std::move(repository)) {}

Result<void> DSPManager::install(const std::string& packagePath) {
    reportProgress("install", 0);

    // 1. Validate package path exists
    if (!fs::exists(packagePath)) {
        return ErrorCode::FirmwarePackageNotFound;
    }

    reportProgress("install", 10);

    // 2. Load package manifest
    DSPPackageMetadata meta;
    auto manifestResult = loadPackageManifest(packagePath, meta);
    if (manifestResult.isError()) {
        return ErrorCode::FirmwareInvalidManifest;
    }

    if (meta.manifest.packageId.empty()) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    reportProgress("install", 30);

    // Check if already installed
    if (m_packages.count(meta.manifest.packageId) > 0) {
        return ErrorCode::AlreadyExists;
    }

    // 3. Check for conflicts (duplicate loaders)
    for (const auto& loader : meta.loaders) {
        if (hasLoaderConflict(loader)) {
            return static_cast<ErrorCode>(DSPError::PackageConflict);
        }
    }

    reportProgress("install", 50);

    // 4. Validate dependencies manually
    if (!meta.manifest.dependencies.empty()) {
        for (const auto& dep : meta.manifest.dependencies) {
            if (!dep.required) continue;
            auto depIt = m_packages.find(dep.packageId);
            if (depIt == m_packages.end()) {
                return static_cast<ErrorCode>(DSPError::DependencyMissing);
            }
            // Check version compatibility
            if (dep.minVersion.major > 0 || dep.minVersion.minor > 0 || dep.minVersion.patch > 0) {
                if (depIt->second.manifest.version < dep.minVersion) {
                    return static_cast<ErrorCode>(DSPError::DependencyMissing);
                }
            }
            if (dep.maxVersion.major > 0 || dep.maxVersion.minor > 0 || dep.maxVersion.patch > 0) {
                if (dep.maxVersion < depIt->second.manifest.version) {
                    return static_cast<ErrorCode>(DSPError::DependencyMissing);
                }
            }
        }
    }

    reportProgress("install", 70);

    // 5. Copy/extract package to user repository
    meta.state = DSPState::Installed;
    meta.origin = DSPOrigin::User;
    meta.installDate = std::chrono::system_clock::now();
    meta.repository = "user";

    // Determine target directory
    std::string targetDir;
    if (!m_repository->userPath().empty()) {
        targetDir = (fs::path(m_repository->userPath()) / meta.manifest.packageId).string();
    } else if (!m_repository->systemPath().empty()) {
        targetDir = (fs::path(m_repository->systemPath()) / meta.manifest.packageId).string();
    } else if (!m_repository->portablePath().empty()) {
        targetDir = (fs::path(m_repository->portablePath()) / meta.manifest.packageId).string();
    } else {
        return static_cast<ErrorCode>(DSPError::InstallFailed);
    }

    // Create target directory
    std::error_code ec;
    if (!fs::create_directories(targetDir, ec) && ec) {
        return static_cast<ErrorCode>(DSPError::InstallFailed);
    }

    // Copy package contents
    try {
        fs::path src(packagePath);
        if (fs::is_directory(src)) {
            for (const auto& entry : fs::recursive_directory_iterator(src, ec)) {
                if (ec) break;
                auto relPath = fs::relative(entry.path(), src);
                auto dest = fs::path(targetDir) / relPath;
                if (entry.is_directory()) {
                    fs::create_directories(dest, ec);
                } else if (entry.is_regular_file()) {
                    fs::copy_file(entry.path(), dest,
                        fs::copy_options::overwrite_existing, ec);
                }
            }
        } else {
            auto dest = fs::path(targetDir) / fs::path(packagePath).filename();
            fs::copy_file(packagePath, dest, fs::copy_options::overwrite_existing, ec);
        }
    } catch (...) {
        return static_cast<ErrorCode>(DSPError::InstallFailed);
    }

    meta.installPath = targetDir;

    reportProgress("install", 90);

    // 6. Register in package index
    m_packages[meta.manifest.packageId] = std::move(meta);

    // Rescan repository
    auto scanResult = m_repository->scan();
    if (scanResult.isError()) {
        return scanResult;
    }

    reportProgress("install", 100);
    return {};
}

Result<void> DSPManager::uninstall(const std::string& packageId) {
    reportProgress("uninstall", 0);

    // 1. Check installed
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        return static_cast<ErrorCode>(DSPError::PackageNotInstalled);
    }

    reportProgress("uninstall", 20);

    // 2. Check dependents manually
    std::vector<std::string> dependents;
    for (const auto& [id, pkg] : m_packages) {
        if (id == packageId) continue;
        for (const auto& dep : pkg.manifest.dependencies) {
            if (dep.packageId == packageId && dep.required) {
                dependents.push_back(id);
            }
        }
    }

    if (!dependents.empty()) {
        return static_cast<ErrorCode>(DSPError::DependencyMissing);
    }

    reportProgress("uninstall", 50);

    // 3. Remove from repository (delete files)
    if (!it->second.installPath.empty()) {
        std::error_code ec;
        fs::remove_all(it->second.installPath, ec);
    }

    // Remove from packages map
    m_packages.erase(it);

    reportProgress("uninstall", 80);

    // 4. Clear cache
    // Rescan repository
    auto scanResult = m_repository->scan();
    if (scanResult.isError()) {
        return scanResult;
    }

    reportProgress("uninstall", 100);
    return {};
}

Result<void> DSPManager::update(const std::string& packageId, const std::string& packagePath) {
    reportProgress("update", 0);

    // Check current package exists
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        return static_cast<ErrorCode>(DSPError::PackageNotInstalled);
    }

    reportProgress("update", 10);

    // 1. Backup current
    auto backupPath = it->second.installPath;

    auto& oldMeta = it->second;

    reportProgress("update", 20);

    // 2. Install new version
    // First make a copy of the package ID and vendor for comparison
    std::string oldPackageId = oldMeta.manifest.packageId;

    auto installResult = install(packagePath);
    if (installResult.isError()) {
        return static_cast<ErrorCode>(DSPError::UpdateFailed);
    }

    reportProgress("update", 60);

    // 3. Verify compatibility
    auto& newMeta = m_packages.at(oldPackageId);
    if (newMeta.manifest.packageId != oldPackageId) {
        // Rollback: remove the newly installed package
        (void)uninstall(newMeta.manifest.packageId);
        return static_cast<ErrorCode>(DSPError::PackageIncompatible);
    }

    reportProgress("update", 80);

    // 4. Remove old version
    if (!backupPath.empty() && backupPath != newMeta.installPath) {
        std::error_code ec;
        fs::remove_all(backupPath, ec);
    }

    newMeta.state = oldMeta.state;

    reportProgress("update", 100);
    return {};
}

Result<void> DSPManager::enable(const std::string& packageId) {
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        return static_cast<ErrorCode>(DSPError::PackageNotInstalled);
    }
    if (it->second.state == DSPState::Corrupted || it->second.state == DSPState::Incompatible) {
        return static_cast<ErrorCode>(DSPError::PackageDisabled);
    }
    it->second.state = DSPState::Enabled;
    return {};
}

Result<void> DSPManager::disable(const std::string& packageId) {
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        return static_cast<ErrorCode>(DSPError::PackageNotInstalled);
    }

    // Check dependents
    for (const auto& [id, pkg] : m_packages) {
        if (id == packageId) continue;
        for (const auto& dep : pkg.manifest.dependencies) {
            if (dep.packageId == packageId && dep.required) {
                return static_cast<ErrorCode>(DSPError::DisableFailed);
            }
        }
    }

    it->second.state = DSPState::Disabled;
    return {};
}

Result<void> DSPManager::repair(const std::string& packageId) {
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        return static_cast<ErrorCode>(DSPError::PackageNotInstalled);
    }

    auto& meta = it->second;

    // Re-validate by re-reading manifest
    if (!meta.installPath.empty()) {
        auto manifestResult = loadPackageManifest(meta.installPath, meta);
        if (manifestResult.isError()) {
            return static_cast<ErrorCode>(DSPError::RepairFailed);
        }
    }

    // Check that all loader files exist
    for (auto& loader : meta.loaders) {
        if (!loader.filePath.empty()) {
            fs::path loaderPath(loader.filePath);
            if (!loaderPath.is_absolute()) {
                loaderPath = fs::path(meta.installPath) / loader.filePath;
            }
            if (!fs::exists(loaderPath)) {
                // Try to find loader file in install path
                bool found = false;
                std::error_code ec;
                for (const auto& entry : fs::recursive_directory_iterator(meta.installPath, ec)) {
                    if (ec) break;
                    if (entry.is_regular_file() && entry.path().filename().string() == loader.fileName) {
                        loader.filePath = entry.path().string();
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return static_cast<ErrorCode>(DSPError::LoaderMissing);
                }
            }
        }
    }

    meta.state = DSPState::Enabled;
    return {};
}

Result<void> DSPManager::verify(const std::string& packageId, DSPValidationLevel level) {
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        return static_cast<ErrorCode>(DSPError::PackageNotInstalled);
    }

    DSPValidator validator;
    validator.setValidationLevel(level);

    std::string checkPath = it->second.installPath;
    if (checkPath.empty()) {
        return static_cast<ErrorCode>(DSPError::MetadataMissing);
    }

    auto report = validator.validate(checkPath, level);
    if (!report.valid) {
        // Mark as corrupted if critical errors
        if (!report.errors.empty()) {
            for (const auto& err : report.errors) {
                if (err.critical) {
                    it->second.state = DSPState::Corrupted;
                    break;
                }
            }
        }
        return static_cast<ErrorCode>(DSPError::ValidationFailed);
    }

    return {};
}

Result<void> DSPManager::reload() {
    reportProgress("reload", 0);

    m_packages.clear();

    // Scan repository
    auto scanResult = m_repository->scan();
    if (scanResult.isError()) {
        return scanResult;
    }

    reportProgress("reload", 50);

    // Reload all packages from repository
    auto allPackages = m_repository->listAll();
    for (const auto& stats : allPackages) {
        auto* pkgMeta = m_repository->byId(stats.packageId);
        if (pkgMeta) {
            m_packages[pkgMeta->manifest.packageId] = *pkgMeta;
        }
    }

    reportProgress("reload", 100);
    return {};
}

std::vector<DSPPackageStatistics> DSPManager::listInstalled() const {
    std::vector<DSPPackageStatistics> result;
    result.reserve(m_packages.size());
    for (const auto& [id, pkg] : m_packages) {
        result.push_back(buildStatistics(pkg));
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.packageId < b.packageId;
    });
    return result;
}

std::vector<DSPPackageStatistics> DSPManager::listAvailable() const {
    auto allStats = m_repository->listAll();
    // Merge with locally tracked state
    for (auto& stats : allStats) {
        auto it = m_packages.find(stats.packageId);
        if (it != m_packages.end()) {
            stats.state = it->second.state;
            stats.installDate = it->second.installDate;
        }
    }
    return allStats;
}

const DSPPackageMetadata* DSPManager::findPackage(const std::string& packageId) const {
    auto it = m_packages.find(packageId);
    if (it != m_packages.end()) {
        return &it->second;
    }
    return m_repository->byId(packageId);
}

std::vector<const DSPPackageMetadata*> DSPManager::findVendor(discovery::Vendor vendor) const {
    return m_repository->byVendor(vendor);
}

std::vector<const DSPChipsetMetadata*> DSPManager::findChipset(const ChipsetId& id) const {
    (void)id;
    return m_repository->allChipsets();
}

std::vector<const DSPLoaderMetadata*> DSPManager::findLoader(const std::string& loaderName) const {
    (void)loaderName;
    return m_repository->allLoaders();
}

std::vector<const DSPLoaderMetadata*> DSPManager::findLoadersForChipset(const ChipsetId& id) const {
    (void)id;
    return m_repository->allLoaders();
}

std::vector<const DSPLoaderMetadata*> DSPManager::findLoadersForProtocol(discovery::ProtocolType proto) const {
    (void)proto;
    return m_repository->allLoaders();
}

DSPPackageStatistics DSPManager::statistics(const std::string& packageId) const {
    auto* pkg = findPackage(packageId);
    if (pkg) {
        return buildStatistics(*pkg);
    }
    DSPPackageStatistics empty;
    empty.packageId = packageId;
    empty.state = DSPState::Unknown;
    return empty;
}

std::vector<std::string> DSPManager::healthReport(const std::string& packageId) const {
    std::vector<std::string> report;
    auto it = m_packages.find(packageId);
    if (it == m_packages.end()) {
        report.push_back("Package not installed: " + packageId);
        return report;
    }

    const auto& meta = it->second;

    // Check install path exists
    if (!meta.installPath.empty() && !fs::exists(meta.installPath)) {
        report.push_back("Install path missing: " + meta.installPath);
    }

    // Check cache
    if (meta.state == DSPState::Corrupted) {
        report.push_back("Package is marked as corrupted");
    }
    if (meta.state == DSPState::Incompatible) {
        report.push_back("Package is marked as incompatible");
    }

    // Check dependencies
    for (const auto& dep : meta.manifest.dependencies) {
        if (dep.required) {
            auto depIt = m_packages.find(dep.packageId);
            if (depIt == m_packages.end()) {
                report.push_back("Missing required dependency: " + dep.packageId);
            } else if (depIt->second.state == DSPState::Disabled ||
                       depIt->second.state == DSPState::Corrupted) {
                report.push_back("Dependency unavailable: " + dep.packageId + " (" +
                    dspStateToString(depIt->second.state) + ")");
            }
        }
    }

    // Check loaders exist
    for (const auto& loader : meta.loaders) {
        if (!loader.filePath.empty()) {
            fs::path lp(loader.filePath);
            if (!lp.is_absolute()) {
                lp = fs::path(meta.installPath) / loader.filePath;
            }
            if (!fs::exists(lp)) {
                report.push_back("Loader file missing: " + loader.loaderId + " (" +
                    loader.filePath + ")");
            }
        }
    }

    return report;
}

size_t DSPManager::installedCount() const noexcept {
    return m_packages.size();
}

size_t DSPManager::enabledCount() const noexcept {
    size_t count = 0;
    for (const auto& [id, pkg] : m_packages) {
        if (pkg.state == DSPState::Enabled) ++count;
    }
    return count;
}

size_t DSPManager::totalLoaderCount() const noexcept {
    size_t count = 0;
    for (const auto& [id, pkg] : m_packages) {
        count += pkg.loaders.size();
    }
    return count;
}

size_t DSPManager::totalChipsetCount() const noexcept {
    size_t count = 0;
    for (const auto& [id, pkg] : m_packages) {
        count += pkg.chipsets.size();
    }
    return count;
}

void DSPManager::setProgressCallback(ProgressCallback cb) {
    m_progressCb = std::move(cb);
}

void DSPManager::reportProgress(const std::string& op, int pct) {
    if (m_progressCb) m_progressCb(op, pct);
}

DSPPackageStatistics DSPManager::buildStatistics(const DSPPackageMetadata& pkg) const {
    DSPPackageStatistics stats;
    stats.packageId = pkg.manifest.packageId;
    stats.name = pkg.manifest.name;
    stats.version = pkg.manifest.version;
    stats.state = pkg.state;
    stats.origin = pkg.origin;
    stats.loaderCount = static_cast<uint32_t>(pkg.loaders.size());
    stats.chipsetCount = static_cast<uint32_t>(pkg.chipsets.size());
    stats.profileCount = static_cast<uint32_t>(pkg.profiles.size());
    stats.quirkCount = static_cast<uint32_t>(pkg.quirks.size());
    stats.installDate = pkg.installDate;
    stats.tags = pkg.manifest.tags;

    // Compute installed size from on-disk size
    if (!pkg.installPath.empty()) {
        std::error_code ec;
        uint64_t totalSize = 0;
        for (const auto& entry : fs::recursive_directory_iterator(pkg.installPath, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) {
                totalSize += entry.file_size();
            }
        }
        if (!ec) {
            stats.installedSize = totalSize;
        }
    }

    // Count dependents
    uint32_t depCount = 0;
    for (const auto& dep : pkg.manifest.dependencies) {
        if (dep.required) ++depCount;
    }
    stats.dependentPackages = depCount;

    return stats;
}

Result<void> DSPManager::loadPackageManifest(const std::string& packagePath, DSPPackageMetadata& meta) {
    auto baseDir = fs::path(packagePath);

    // If path is a file, use its parent directory
    if (fs::is_regular_file(baseDir)) {
        baseDir = baseDir.parent_path();
    }

    // Read manifest.json
    auto manifestPath = (baseDir / "manifest.json").string();
    if (!fs::exists(manifestPath)) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    auto manifestContent = readFileContents(manifestPath);
    if (manifestContent.empty()) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    nlohmann::json manifestParser;
    try {
        manifestParser = nlohmann::json::parse(manifestContent);
    } catch (...) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }
    if (!manifestParser.is_object()) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    // Parse basic manifest fields
    meta.manifest.formatVersion = manifestParser.value("formatVersion", std::string{});
    meta.manifest.packageId = manifestParser.value("packageId", std::string{});
    meta.manifest.name = manifestParser.value("name", std::string{});
    meta.manifest.vendor = manifestParser.value("vendor", std::string{});
    meta.manifest.description = manifestParser.value("description", std::string{});
    meta.manifest.license = manifestParser.value("license", std::string{});
    meta.manifest.minCoreVersion = manifestParser.value("minCoreVersion", std::string{});
    meta.manifest.maxCoreVersion = manifestParser.value("maxCoreVersion", std::string{});
    meta.manifest.checksumAlgorithm = manifestParser.value("checksumAlgorithm", std::string{});
    meta.manifest.signatureAlgorithm = manifestParser.value("signatureAlgorithm", std::string{});
    meta.manifest.fileSize = manifestParser.value("fileSize", uint64_t{0});
    meta.manifest.checksum = static_cast<uint32_t>(manifestParser.value("checksum", uint64_t{0}));
    meta.manifest.requiresSignature = manifestParser.value("requiresSignature", false);
    meta.manifest.compressLoaders = manifestParser.value("compressLoaders", false);

    // Version
    auto versionStr = manifestParser.value("version", std::string{});
    if (!versionStr.empty()) {
        std::regex versionRegex(R"((\d+)\.(\d+)\.(\d+))");
        std::smatch m;
        if (std::regex_match(versionStr, m, versionRegex)) {
            auto r1 = fromCharsUint32(m[1].str());
            auto r2 = fromCharsUint32(m[2].str());
            auto r3 = fromCharsUint32(m[3].str());
            if (r1.ok) meta.manifest.version.major = r1.value;
            if (r2.ok) meta.manifest.version.minor = r2.value;
            if (r3.ok) meta.manifest.version.patch = r3.value;
        }
    }

    // Tags
    {
        auto arr = manifestParser.value("tags", nlohmann::json::array());
        if (arr.is_array()) {
            for (const auto& item : arr) if (item.is_string()) meta.manifest.tags.push_back(item.get<std::string>());
        }
    }

    // Authors
    {
        auto arr = manifestParser.value("authors", nlohmann::json::array());
        if (arr.is_array()) {
            for (const auto& item : arr) if (item.is_string()) meta.manifest.authors.push_back(item.get<std::string>());
        }
    }

    // Dependencies
    {
        auto deps = manifestParser.value("dependencies", nlohmann::json::object());
        if (deps.is_object()) {
            for (auto it = deps.begin(); it != deps.end(); ++it) {
                DSPDependency dep;
                dep.packageId = it.key();
                dep.name = it.key();
                dep.required = true;
                if (it.value().is_string()) {
                    std::string depVer = it.value().get<std::string>();
                    if (!depVer.empty()) {
                        std::regex verRange(R"((\d+\.\d+\.\d+)\s*-\s*(\d+\.\d+\.\d+))");
                        std::smatch m;
                        if (std::regex_match(depVer, m, verRange)) {
                            std::regex ve(R"((\d+)\.(\d+)\.(\d+))");
                            std::smatch vm;
                            auto minStr = m[1].str();
                            auto maxStr = m[2].str();
                            if (std::regex_match(minStr, vm, ve)) {
                                auto r1 = fromCharsUint32(vm[1].str());
                                auto r2 = fromCharsUint32(vm[2].str());
                                auto r3 = fromCharsUint32(vm[3].str());
                                if (r1.ok) dep.minVersion.major = r1.value;
                                if (r2.ok) dep.minVersion.minor = r2.value;
                                if (r3.ok) dep.minVersion.patch = r3.value;
                            }
                            if (std::regex_match(maxStr, vm, ve)) {
                                auto r1 = fromCharsUint32(vm[1].str());
                                auto r2 = fromCharsUint32(vm[2].str());
                                auto r3 = fromCharsUint32(vm[3].str());
                                if (r1.ok) dep.maxVersion.major = r1.value;
                                if (r2.ok) dep.maxVersion.minor = r2.value;
                                if (r3.ok) dep.maxVersion.patch = r3.value;
                            }
                        } else {
                            std::regex ve(R"((\d+)\.(\d+)\.(\d+))");
                            std::smatch vm;
                            if (std::regex_match(depVer, vm, ve)) {
                                auto r1 = fromCharsUint32(vm[1].str());
                                auto r2 = fromCharsUint32(vm[2].str());
                                auto r3 = fromCharsUint32(vm[3].str());
                                if (r1.ok) dep.minVersion.major = r1.value;
                                if (r2.ok) dep.minVersion.minor = r2.value;
                                if (r3.ok) dep.minVersion.patch = r3.value;
                                dep.maxVersion = dep.minVersion;
                            }
                        }
                    }
                }
                meta.manifest.dependencies.push_back(std::move(dep));
            }
        }
    }

    // Dates
    auto buildDate = manifestParser.value("buildDate", std::string{});
    if (!buildDate.empty()) {
        meta.manifest.buildDate = stringToTimePoint(buildDate);
    }
    auto releaseDate = manifestParser.value("releaseDate", std::string{});
    if (!releaseDate.empty()) {
        meta.manifest.releaseDate = stringToTimePoint(releaseDate);
    }

    // Signature
    auto sigStr = manifestParser.value("signature", std::string{});
    if (!sigStr.empty()) {
        meta.manifest.signature.assign(sigStr.begin(), sigStr.end());
    }

    // Read vendor.json if exists
    auto vendorPath = (baseDir / "vendor.json").string();
    if (fs::exists(vendorPath)) {
        auto vendorContent = readFileContents(vendorPath);
        if (!vendorContent.empty()) {
            try {
                auto vendorJson = nlohmann::json::parse(vendorContent);
                if (vendorJson.is_object()) {
                    meta.vendor.vendorName = vendorJson.value("vendorName", std::string{});
                    meta.vendor.displayName = vendorJson.value("displayName", std::string{});
                    meta.vendor.website = vendorJson.value("website", std::string{});
                    meta.vendor.defaultProtocol = vendorJson.value("defaultProtocol", std::string{});
                    meta.vendor.defaultBootMode = vendorJson.value("defaultBootMode", std::string{});
                    meta.vendor.defaultTransport = vendorJson.value("defaultTransport", std::string{});
                    {
                        auto arr = vendorJson.value("aliases", nlohmann::json::array());
                        if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) meta.vendor.aliases.push_back(item.get<std::string>());
                    }
                    {
                        auto arr = vendorJson.value("supportedChipsets", nlohmann::json::array());
                        if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) meta.vendor.supportedChipsets.push_back(item.get<std::string>());
                    }
                    {
                        auto arr = vendorJson.value("supportedBootModes", nlohmann::json::array());
                        if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) meta.vendor.supportedBootModes.push_back(item.get<std::string>());
                    }
                    {
                        auto arr = vendorJson.value("supportedProtocols", nlohmann::json::array());
                        if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) meta.vendor.supportedProtocols.push_back(item.get<std::string>());
                    }
                    {
                        auto arr = vendorJson.value("supportedTransports", nlohmann::json::array());
                        if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) meta.vendor.supportedTransports.push_back(item.get<std::string>());
                    }

                    auto vendorId = vendorJson.value("vendorId", std::string{});
                    if (!vendorId.empty()) {
                        meta.vendor.vendorId = stringToVendor(vendorId);
                    }

                    auto props = vendorJson.value("properties", nlohmann::json::object());
                    if (props.is_object()) {
                        for (auto it = props.begin(); it != props.end(); ++it) {
                            if (it.value().is_string())
                                meta.vendor.properties[it.key()] = it.value().get<std::string>();
                        }
                    }
                }
            } catch (...) {}
        }
    }

    // Read chipsets.json if exists
    auto chipsetsPath = (baseDir / "chipsets.json").string();
    if (fs::exists(chipsetsPath)) {
        auto chipsetsContent = readFileContents(chipsetsPath);
        if (!chipsetsContent.empty()) {
            try {
                auto chipsetsJson = nlohmann::json::parse(chipsetsContent);
                if (chipsetsJson.is_array()) {
                    for (const auto& csItem : chipsetsJson) {
                        if (!csItem.is_object()) continue;
                        DSPChipsetMetadata cs;
                        cs.id.vendor = csItem.value("vendor", std::string{});
                        cs.id.family = csItem.value("family", std::string{});
                        cs.id.variant = csItem.value("variant", std::string{});
                        cs.displayName = csItem.value("displayName", std::string{});
                        cs.manufacturer = csItem.value("manufacturer", std::string{});
                        cs.architecture = csItem.value("architecture", std::string{});
                        cs.defaultProgrammer = csItem.value("defaultProgrammer", std::string{});
                        cs.maxMemoryMB = static_cast<uint32_t>(csItem.value("maxMemoryMB", uint64_t{0}));
                        cs.maxFlashSizeMB = static_cast<uint32_t>(csItem.value("maxFlashSizeMB", uint64_t{0}));
                        cs.hasTrustZone = csItem.value("hasTrustZone", false);
                        cs.hasSecureBoot = csItem.value("hasSecureBoot", false);
                        cs.hasDebugPort = csItem.value("hasDebugPort", false);
                        {
                            auto arr = csItem.value("variants", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) cs.variants.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = csItem.value("families", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) cs.families.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = csItem.value("predecessors", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) cs.predecessors.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = csItem.value("successors", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) cs.successors.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = csItem.value("compatibleLoaders", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) cs.compatibleLoaders.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = csItem.value("knownIssues", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) cs.knownIssues.push_back(item.get<std::string>());
                        }

                        {
                            auto bootModesArr = csItem.value("supportedBootModes", nlohmann::json::array());
                            if (bootModesArr.is_array()) for (const auto& bm : bootModesArr) if (bm.is_string()) cs.supportedBootModes.push_back(stringToBootMode(bm.get<std::string>()));
                        }
                        {
                            auto protocolsArr = csItem.value("supportedProtocols", nlohmann::json::array());
                            if (protocolsArr.is_array()) for (const auto& pr : protocolsArr) if (pr.is_string()) cs.supportedProtocols.push_back(stringToProtocol(pr.get<std::string>()));
                        }
                        {
                            auto storageArr = csItem.value("supportedStorage", nlohmann::json::array());
                            if (storageArr.is_array()) for (const auto& st : storageArr) if (st.is_string()) cs.supportedStorage.push_back(stringToStorageType(st.get<std::string>()));
                        }

                        auto csProps = csItem.value("properties", nlohmann::json::object());
                        if (csProps.is_object()) {
                            for (auto it = csProps.begin(); it != csProps.end(); ++it) {
                                if (it.value().is_string())
                                    cs.properties[it.key()] = it.value().get<std::string>();
                            }
                        }

                        auto relDate = csItem.value("releaseDate", std::string{});
                        if (!relDate.empty()) {
                            cs.releaseDate = stringToTimePoint(relDate);
                        }

                        meta.chipsets.push_back(std::move(cs));
                    }
                }
            } catch (...) {}
        }
    }

    // Read loaders from loaders directory or loaders.json
    auto loadersDir = (baseDir / "loaders").string();
    auto loadersJsonPath = (baseDir / "loaders.json").string();

    if (fs::exists(loadersJsonPath)) {
        auto loadersContent = readFileContents(loadersJsonPath);
        if (!loadersContent.empty()) {
            try {
                auto loadersJson = nlohmann::json::parse(loadersContent);
                if (loadersJson.is_array()) {
                    for (const auto& ldItem : loadersJson) {
                        if (!ldItem.is_object()) continue;
                        DSPLoaderMetadata ld;
                        ld.loaderId = ldItem.value("loaderId", std::string{});
                        ld.name = ldItem.value("name", std::string{});
                        ld.fileName = ldItem.value("fileName", std::string{});
                        ld.filePath = ldItem.value("filePath", std::string{});
                        ld.description = ldItem.value("description", std::string{});
                        ld.hashAlgorithm = ldItem.value("hashAlgorithm", std::string{});
                        ld.version.major = static_cast<uint32_t>(ldItem.value("versionMajor", uint64_t{0}));
                        ld.version.minor = static_cast<uint32_t>(ldItem.value("versionMinor", uint64_t{0}));
                        ld.version.patch = static_cast<uint32_t>(ldItem.value("versionPatch", uint64_t{0}));
                        ld.priority = static_cast<uint32_t>(ldItem.value("priority", uint64_t{0}));
                        ld.originalSize = ldItem.value("originalSize", uint64_t{0});
                        ld.compressedSize = ldItem.value("compressedSize", uint64_t{0});
                        ld.isCompressed = ldItem.value("isCompressed", false);
                        ld.isFallback = ldItem.value("isFallback", false);
                        ld.isPreferred = ldItem.value("isPreferred", false);
                        ld.isSigned = ldItem.value("isSigned", false);
                        ld.requiresAuthentication = ldItem.value("requiresAuthentication", false);
                        {
                            auto arr = ldItem.value("tags", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) ld.tags.push_back(item.get<std::string>());
                        }

                        {
                            auto arr = ldItem.value("compatibleVendors", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) ld.compatibleVendors.push_back(item.get<std::string>());
                        }

                        {
                            auto arr = ldItem.value("protocols", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) ld.protocols.push_back(stringToProtocol(item.get<std::string>()));
                        }

                        {
                            auto arr = ldItem.value("bootModes", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) ld.bootModes.push_back(stringToBootMode(item.get<std::string>()));
                        }

                        auto requiredMode = ldItem.value("requiredBootMode", std::string{});
                        if (!requiredMode.empty()) {
                            ld.requiredBootMode = stringToBootMode(requiredMode);
                        }

                        // Parse compatible chipsets array of objects
                        {
                            auto compatArr = ldItem.value("compatibleChipsets", nlohmann::json::array());
                            if (compatArr.is_array()) {
                                for (const auto& cItem : compatArr) {
                                    if (cItem.is_object()) {
                                        ChipsetId cid;
                                        cid.vendor = cItem.value("vendor", std::string{});
                                        cid.family = cItem.value("family", std::string{});
                                        cid.variant = cItem.value("variant", std::string{});
                                        if (!cid.vendor.empty() && !cid.family.empty()) {
                                            ld.compatibleChipsets.push_back(std::move(cid));
                                        }
                                    }
                                }
                            }
                        }

                        // Expected hash
                        auto hashStr = ldItem.value("expectedHash", std::string{});
                        if (!hashStr.empty()) {
                            ld.expectedHash.assign(hashStr.begin(), hashStr.end());
                        }

                        {
                            auto ldProps = ldItem.value("properties", nlohmann::json::object());
                            if (ldProps.is_object()) {
                                for (auto it = ldProps.begin(); it != ldProps.end(); ++it) {
                                    if (it.value().is_string())
                                        ld.properties[it.key()] = it.value().get<std::string>();
                                }
                            }
                        }

                        // Set default file path if not specified but file exists in loaders dir
                        if (ld.filePath.empty() && !ld.fileName.empty() && fs::exists(loadersDir)) {
                            auto loaderFile = fs::path(loadersDir) / ld.fileName;
                            if (fs::exists(loaderFile)) {
                                ld.filePath = loaderFile.string();
                            }
                        }

                        meta.loaders.push_back(std::move(ld));
                    }
                }
            } catch (...) {}
        }
    }

    // Also scan individual loader files in loaders directory
    if (fs::exists(loadersDir) && fs::is_directory(loadersDir)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(loadersDir, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                auto fname = entry.path().filename().string();
                // Check if this file already has a metadata entry
                bool hasEntry = false;
                for (auto& ld : meta.loaders) {
                    if (ld.fileName == fname || ld.filePath == entry.path().string()) {
                        hasEntry = true;
                        break;
                    }
                }
                if (!hasEntry) {
                    DSPLoaderMetadata autoLd;
                    autoLd.loaderId = fname;
                    autoLd.name = entry.path().stem().string();
                    autoLd.fileName = fname;
                    autoLd.filePath = entry.path().string();
                    autoLd.version.major = 1;
                    autoLd.version.minor = 0;
                    autoLd.version.patch = 0;
                    autoLd.originalSize = entry.file_size();
                    meta.loaders.push_back(std::move(autoLd));
                }
            }
        }
    }

    // Read bootmodes.json if exists
    auto bootModesPath = (baseDir / "bootmodes.json").string();
    if (fs::exists(bootModesPath)) {
        auto bmContent = readFileContents(bootModesPath);
        if (!bmContent.empty()) {
            try {
                auto bmJson = nlohmann::json::parse(bmContent);
                if (bmJson.is_array()) {
                    for (const auto& bmItem : bmJson) {
                        if (!bmItem.is_object()) continue;
                        DSPBootModeMetadata bm;
                        bm.bootModeId = bmItem.value("bootModeId", std::string{});
                        bm.displayName = bmItem.value("displayName", std::string{});
                        bm.description = bmItem.value("description", std::string{});
                        bm.defaultTimeoutMs = static_cast<uint32_t>(bmItem.value("defaultTimeoutMs", uint64_t{0}));
                        bm.requiresAuth = bmItem.value("requiresAuth", false);
                        bm.requireUsb = bmItem.value("requireUsb", false);
                        bm.requireSerial = bmItem.value("requireSerial", false);
                        bm.requireTcp = bmItem.value("requireTcp", false);

                        {
                            auto arr = bmItem.value("supportedProtocols", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) bm.supportedProtocols.push_back(stringToProtocol(item.get<std::string>()));
                        }
                        {
                            auto arr = bmItem.value("requiredKeys", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) bm.requiredKeys.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = bmItem.value("quirks", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) bm.quirks.push_back(item.get<std::string>());
                        }

                        auto bmStr2 = bmItem.value("bootMode", std::string{});
                        if (!bmStr2.empty()) {
                            bm.bootMode = stringToBootMode(bmStr2);
                        }

                        auto bmProps = bmItem.value("properties", nlohmann::json::object());
                        if (bmProps.is_object()) {
                            for (auto it = bmProps.begin(); it != bmProps.end(); ++it) {
                                if (it.value().is_string())
                                    bm.properties[it.key()] = it.value().get<std::string>();
                            }
                        }

                        meta.bootModes.push_back(std::move(bm));
                    }
                }
            } catch (...) {}
        }
    }

    // Read transports.json if exists
    auto transportsPath = (baseDir / "transports.json").string();
    if (fs::exists(transportsPath)) {
        auto tpContent = readFileContents(transportsPath);
        if (!tpContent.empty()) {
            try {
                auto tpJson = nlohmann::json::parse(tpContent);
                if (tpJson.is_array()) {
                    for (const auto& tpItem : tpJson) {
                        if (!tpItem.is_object()) continue;
                        DSPTransportMetadata tp;
                        tp.transportId = tpItem.value("transportId", std::string{});
                        tp.displayName = tpItem.value("displayName", std::string{});
                        tp.description = tpItem.value("description", std::string{});
                        tp.defaultBaudRate = static_cast<uint32_t>(tpItem.value("defaultBaudRate", uint64_t{0}));
                        tp.defaultTimeoutMs = static_cast<uint32_t>(tpItem.value("defaultTimeoutMs", uint64_t{0}));
                        tp.maxPacketSize = static_cast<uint32_t>(tpItem.value("maxPacketSize", uint64_t{0}));
                        tp.supportsHotplug = tpItem.value("supportsHotplug", false);
                        tp.supportsAsync = tpItem.value("supportsAsync", false);

                        {
                            auto arr = tpItem.value("supportedBaudRates", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) {
                                auto r = fromCharsUint32(item.get<std::string>());
                                if (r.ok) tp.supportedBaudRates.push_back(r.value);
                            }
                        }

                        auto tpProps = tpItem.value("properties", nlohmann::json::object());
                        if (tpProps.is_object()) {
                            for (auto it = tpProps.begin(); it != tpProps.end(); ++it) {
                                if (it.value().is_string())
                                    tp.properties[it.key()] = it.value().get<std::string>();
                            }
                        }

                        auto tpType = tpItem.value("transportType", std::string{});
                        if (tpType == "USB") tp.transportType = discovery::TransportType::USB;
                        else if (tpType == "Serial") tp.transportType = discovery::TransportType::Serial;
                        else if (tpType == "TCP") tp.transportType = discovery::TransportType::TCP;
                        else if (tpType == "Virtual") tp.transportType = discovery::TransportType::Virtual;

                        meta.transports.push_back(std::move(tp));
                    }
                }
            } catch (...) {}
        }
    }

    // Read protocols.json if exists
    auto protocolsPath = (baseDir / "protocols.json").string();
    if (fs::exists(protocolsPath)) {
        auto prContent = readFileContents(protocolsPath);
        if (!prContent.empty()) {
            try {
                auto prJson = nlohmann::json::parse(prContent);
                if (prJson.is_array()) {
                    for (const auto& prItem : prJson) {
                        if (!prItem.is_object()) continue;
                        DSPProtocolMetadata pr;
                        pr.protocolId = prItem.value("protocolId", std::string{});
                        pr.displayName = prItem.value("displayName", std::string{});
                        pr.description = prItem.value("description", std::string{});
                        pr.defaultTimeoutMs = static_cast<uint32_t>(prItem.value("defaultTimeoutMs", uint64_t{0}));
                        pr.requiresProgrammer = prItem.value("requiresProgrammer", false);
                        pr.supportsBackup = prItem.value("supportsBackup", false);
                        pr.supportsVerify = prItem.value("supportsVerify", false);
                        pr.supportsResume = prItem.value("supportsResume", false);

                        auto minVer = prItem.value("minVersion", std::string{});
                        if (!minVer.empty()) {
                            std::regex ve(R"((\d+)\.(\d+)\.(\d+))");
                            std::smatch vm;
                            if (std::regex_match(minVer, vm, ve)) {
                                auto r1 = fromCharsUint32(vm[1].str());
                                auto r2 = fromCharsUint32(vm[2].str());
                                auto r3 = fromCharsUint32(vm[3].str());
                                if (r1.ok) pr.minVersion.major = r1.value;
                                if (r2.ok) pr.minVersion.minor = r2.value;
                                if (r3.ok) pr.minVersion.patch = r3.value;
                            }
                        }
                        auto maxVer = prItem.value("maxVersion", std::string{});
                        if (!maxVer.empty()) {
                            std::regex ve(R"((\d+)\.(\d+)\.(\d+))");
                            std::smatch vm;
                            if (std::regex_match(maxVer, vm, ve)) {
                                auto r1 = fromCharsUint32(vm[1].str());
                                auto r2 = fromCharsUint32(vm[2].str());
                                auto r3 = fromCharsUint32(vm[3].str());
                                if (r1.ok) pr.maxVersion.major = r1.value;
                                if (r2.ok) pr.maxVersion.minor = r2.value;
                                if (r3.ok) pr.maxVersion.patch = r3.value;
                            }
                        }

                        {
                            auto arr = prItem.value("requiredLoaders", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) pr.requiredLoaders.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = prItem.value("optionalLoaders", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) pr.optionalLoaders.push_back(item.get<std::string>());
                        }
                        {
                            auto arr = prItem.value("quirks", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) pr.quirks.push_back(item.get<std::string>());
                        }

                        auto prType = prItem.value("protocolType", std::string{});
                        if (!prType.empty()) {
                            pr.protocolType = stringToProtocol(prType);
                        }

                        {
                            auto arr = prItem.value("supportedBootModes", nlohmann::json::array());
                            if (arr.is_array()) for (const auto& item : arr) if (item.is_string()) pr.supportedBootModes.push_back(stringToBootMode(item.get<std::string>()));
                        }

                        auto prProps = prItem.value("properties", nlohmann::json::object());
                        if (prProps.is_object()) {
                            for (auto it = prProps.begin(); it != prProps.end(); ++it) {
                                if (it.value().is_string())
                                    pr.properties[it.key()] = it.value().get<std::string>();
                            }
                        }

                        meta.protocols.push_back(std::move(pr));
                    }
                }
            } catch (...) {}
        }
    }

    // Read quirks.json if exists
    auto quirksPath = (baseDir / "quirks.json").string();
    if (fs::exists(quirksPath)) {
        meta.quirks = readFileContents(quirksPath) == "[]" ? std::vector<std::string>{} :
            std::vector<std::string>{quirksPath};
    }

    // Read profiles directory if exists
    auto profilesDir = (baseDir / "profiles").string();
    if (fs::exists(profilesDir) && fs::is_directory(profilesDir)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(profilesDir, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) {
                meta.profiles.push_back(entry.path().filename().string());
            }
        }
    }

    // DSP format version
    std::string dspVer = manifestParser.value("dspFormatVersion", std::string{});
    if (!dspVer.empty()) {
        std::regex ve(R"((\d+)\.(\d+)\.(\d+))");
        std::smatch vm;
        if (std::regex_match(dspVer, vm, ve)) {
            auto r1 = fromCharsUint32(vm[1].str());
            auto r2 = fromCharsUint32(vm[2].str());
            auto r3 = fromCharsUint32(vm[3].str());
            if (r1.ok) meta.dspFormatVersion.major = r1.value;
            if (r2.ok) meta.dspFormatVersion.minor = r2.value;
            if (r3.ok) meta.dspFormatVersion.patch = r3.value;
        }
    }

    meta.packageFileName = fs::path(packagePath).filename().string();

    return {};
}

bool DSPManager::hasLoaderConflict(const DSPLoaderMetadata& loader) const {
    for (const auto& [id, pkg] : m_packages) {
        for (const auto& existing : pkg.loaders) {
            if (existing.loaderId == loader.loaderId) {
                return true;
            }
        }
    }
    return false;
}

} // namespace dsp
} // namespace mbootcore
