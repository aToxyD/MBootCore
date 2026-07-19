#include "mbootcore/dsp/DSPValidator.hpp"
#include "mbootcore/domain/Error.hpp"

#include "SafeParser.hpp"
#include <charconv>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_set>

namespace mbootcore {
namespace dsp {

namespace {

namespace fs = std::filesystem;

std::string readFileToString(const std::string& path) {
    fs::path fpath(path);
    if (!fs::exists(fpath) || !fs::is_regular_file(fpath)) return {};
    std::ifstream ifs(fpath, std::ios::binary);
    if (!ifs) return {};
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

const std::unordered_set<std::string> kRequiredManifestKeys = {
    "packageId", "name", "version", "vendor"
};

const std::unordered_set<std::string> kManifestSchemaKeys = {
    "packageId", "name", "version", "vendor", "description",
    "authors", "license", "tags", "dependencies",
    "minCoreVersion", "maxCoreVersion", "formatVersion"
};

bool versionStringToParts(const std::string& ver, uint32_t& major, uint32_t& minor, uint32_t& patch) {
    major = 0; minor = 0; patch = 0;
    if (ver.empty()) return false;
    const char* first = ver.data();
    const char* last = ver.data() + ver.size();
    const char* p = first;
    auto [ptr1, ec1] = std::from_chars(p, last, major);
    if (ec1 != std::errc{}) return false;
    p = ptr1;
    if (p < last && *p == '.') ++p;
    auto [ptr2, ec2] = std::from_chars(p, last, minor);
    if (ec2 != std::errc{}) return true;
    p = ptr2;
    if (p < last && *p == '.') ++p;
    auto [ptr3, ec3] = std::from_chars(p, last, patch);
    (void)ptr3; (void)ec3;
    return true;
}

} // anonymous namespace

DSPValidator::DSPValidator() = default;

void DSPValidator::setValidationLevel(DSPValidationLevel level) noexcept {
    m_level = level;
}

DSPValidationLevel DSPValidator::validationLevel() const noexcept {
    return m_level;
}

bool DSPValidator::checkFileExists(const std::string& path) const {
    return fs::exists(fs::path(path));
}

bool DSPValidator::checkJsonSchema(const std::string& jsonPath, const std::string& schemaName) const {
    std::string content = readFileToString(jsonPath);
    if (content.empty()) return false;

    try {
        auto json = nlohmann::json::parse(content);

        if (schemaName == "manifest") {
            for (const auto& key : kRequiredManifestKeys) {
                if (!json.contains(key)) return false;
            }
        } else if (schemaName == "vendor") {
            if (!json.contains("vendorName")) return false;
        } else if (schemaName == "checksums") {
            return !json.empty();
        }

        return true;
    } catch (...) {
        return false;
    }
}

uint32_t DSPValidator::computeFileChecksum(const std::string& path) const {
    std::string data = readFileToString(path);
    if (data.empty()) return 0;

    uint32_t hash = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        hash ^= static_cast<uint32_t>(static_cast<uint8_t>(data[i])) << ((i % 4) * 8);
        if (i % 4 == 3) {
            hash = (hash << 13) | (hash >> 19);
            hash ^= 0x55555555;
        }
    }
    return hash;
}

bool DSPValidator::verifyChecksum(const std::string& path, uint32_t expected) const {
    return computeFileChecksum(path) == expected;
}

ValidationReport DSPValidator::validate(const std::string& packagePath, DSPValidationLevel level) {
    ValidationReport report;
    report.valid = true;
    report.packageId = fs::path(packagePath).filename().string();

    if (!checkFileExists(packagePath)) {
        ValidationError err;
        err.field = "packagePath";
        err.message = "Package path does not exist: " + packagePath;
        err.error = DSPError::PackageNotFound;
        err.critical = true;
        report.errors.push_back(std::move(err));
        report.valid = false;
        return report;
    }

    auto manifestReport = validateManifest(packagePath);
    if (!manifestReport.valid) {
        report.valid = false;
        report.errors.insert(report.errors.end(),
            manifestReport.errors.begin(), manifestReport.errors.end());
    }
    report.warnings.insert(report.warnings.end(),
        manifestReport.warnings.begin(), manifestReport.warnings.end());
    report.info.insert(report.info.end(),
        manifestReport.info.begin(), manifestReport.info.end());
    report.filesChecked += manifestReport.filesChecked;

    if (level == DSPValidationLevel::None) return report;

    auto metadataPath = (fs::path(packagePath) / "metadata.json").string();
    DSPPackageMetadata dummyMeta;
    if (checkFileExists(metadataPath)) {
        report.filesChecked++;
        auto metaReport = validateMetadata(dummyMeta);
        if (!metaReport.valid) {
            report.valid = false;
            report.errors.insert(report.errors.end(),
                metaReport.errors.begin(), metaReport.errors.end());
        }
    }

    if (level >= DSPValidationLevel::Basic) {
        auto checksumReport = validateChecksums(packagePath);
        if (!checksumReport.valid) {
            report.valid = false;
            report.errors.insert(report.errors.end(),
                checksumReport.errors.begin(), checksumReport.errors.end());
        }
        report.warnings.insert(report.warnings.end(),
            checksumReport.warnings.begin(), checksumReport.warnings.end());
        report.checksumsVerified += checksumReport.checksumsVerified;
        report.filesChecked += checksumReport.filesChecked;
    }

    if (level >= DSPValidationLevel::Full) {
        auto schemaReport = validateSchema(packagePath);
        if (!schemaReport.valid) {
            report.valid = false;
            report.errors.insert(report.errors.end(),
                schemaReport.errors.begin(), schemaReport.errors.end());
        }
        report.warnings.insert(report.warnings.end(),
            schemaReport.warnings.begin(), schemaReport.warnings.end());
        report.filesChecked += schemaReport.filesChecked;

        auto signatureReport = validateSignature(packagePath);
        if (!signatureReport.valid) {
            report.valid = false;
            report.errors.insert(report.errors.end(),
                signatureReport.errors.begin(), signatureReport.errors.end());
        }
    }

    return report;
}

ValidationReport DSPValidator::validateMetadata(const DSPPackageMetadata& metadata) {
    ValidationReport report;
    report.valid = true;
    report.packageId = metadata.manifest.packageId;
    report.packageVersion = metadata.manifest.version;

    if (metadata.manifest.packageId.empty()) {
        ValidationError err;
        err.field = "manifest.packageId";
        err.message = "Package ID is empty";
        err.error = DSPError::MetadataMissing;
        err.critical = true;
        report.errors.push_back(std::move(err));
        report.valid = false;
    }

    if (metadata.manifest.name.empty()) {
        ValidationError err;
        err.field = "manifest.name";
        err.message = "Package name is empty";
        err.error = DSPError::MetadataMissing;
        err.critical = false;
        report.errors.push_back(std::move(err));
        report.valid = false;
    }

    if (metadata.manifest.vendor.empty()) {
        ValidationError err;
        err.field = "manifest.vendor";
        err.message = "Package vendor is empty";
        err.error = DSPError::MetadataMissing;
        err.critical = false;
        report.errors.push_back(std::move(err));
        report.valid = false;
    }

    if (metadata.manifest.description.empty()) {
        report.warnings.push_back("Package description is empty");
    }

    if (metadata.manifest.authors.empty()) {
        report.warnings.push_back("Package has no authors listed");
    }

    if (metadata.manifest.license.empty()) {
        report.info.push_back("Package license not specified");
    }

    return report;
}

ValidationReport DSPValidator::validateManifest(const std::string& packagePath) {
    ValidationReport report;
    report.valid = true;
    report.packageId = fs::path(packagePath).filename().string();

    fs::path manifestPath = fs::path(packagePath) / "manifest.json";
    if (!checkFileExists(manifestPath.string())) {
        ValidationError err;
        err.field = "manifest.json";
        err.message = "manifest.json not found in package";
        err.error = DSPError::ManifestInvalid;
        err.critical = true;
        report.errors.push_back(std::move(err));
        report.valid = false;
        return report;
    }
    report.filesChecked++;

    if (!checkJsonSchema(manifestPath.string(), "manifest")) {
        ValidationError err;
        err.field = "manifest.json";
        err.message = "manifest.json missing required fields (packageId, name, version, vendor)";
        err.error = DSPError::ManifestInvalid;
        err.critical = true;
        report.errors.push_back(std::move(err));
        report.valid = false;
        return report;
    }

    std::string content = readFileToString(manifestPath.string());
    try {
        auto json = nlohmann::json::parse(content);

        std::string packageId = json.value("packageId", "");
        if (!packageId.empty()) {
            report.packageId = packageId;
        }

        std::string version = json.value("version", "");
        if (!version.empty()) {
            uint32_t maj, min, pat;
            if (versionStringToParts(version, maj, min, pat)) {
                report.packageVersion = {maj, min, pat};
            } else {
                report.warnings.push_back("Invalid version format: " + version);
            }
        }
    } catch (...) {
    }

    report.info.push_back("Manifest validated successfully");
    return report;
}

ValidationReport DSPValidator::validateChecksums(const std::string& packagePath) {
    ValidationReport report;
    report.valid = true;
    report.packageId = fs::path(packagePath).filename().string();

    fs::path checksumPath = fs::path(packagePath) / "checksums.json";
    if (!checkFileExists(checksumPath.string())) {
        report.warnings.push_back("No checksums.json found, skipping checksum verification");
        return report;
    }
    report.filesChecked++;

    std::string content = readFileToString(checksumPath.string());
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(content);
    } catch (...) {
        ValidationError err;
        err.field = "checksums.json";
        err.message = "Invalid checksums.json format";
        err.error = DSPError::ChecksumMismatch;
        err.critical = false;
        report.errors.push_back(std::move(err));
        report.valid = false;
        return report;
    }

    for (auto& [filePath, expectedHashVal] : json.items()) {
        if (!expectedHashVal.is_string()) continue;
        std::string expectedHashStr = expectedHashVal.get<std::string>();
        fs::path targetFile = fs::path(packagePath) / filePath;
        if (!checkFileExists(targetFile.string())) {
            report.warnings.push_back("File listed in checksums.json not found: " + filePath);
            continue;
        }
        report.filesChecked++;

        uint32_t expected = 0;
        {
            auto r = fromCharsUint32(expectedHashStr);
            if (r.ok) {
                expected = r.value;
            } else {
                expected = static_cast<uint32_t>(std::hash<std::string>{}(expectedHashStr) & 0xFFFFFFFF);
            }
        }

        if (!verifyChecksum(targetFile.string(), expected)) {
            ValidationError err;
            err.field = filePath;
            err.message = "Checksum mismatch for: " + filePath;
            err.error = DSPError::ChecksumMismatch;
            err.critical = false;
            report.errors.push_back(std::move(err));
            report.valid = false;
        } else {
            report.checksumsVerified++;
        }
    }

    if (report.checksumsVerified > 0) {
        report.info.push_back("Verified " + std::to_string(report.checksumsVerified) + " checksums");
    }

    return report;
}

ValidationReport DSPValidator::validateLoaders(const std::string& packagePath, const DSPPackageMetadata& metadata) {
    ValidationReport report;
    report.valid = true;
    report.packageId = metadata.manifest.packageId;
    report.packageVersion = metadata.manifest.version;

    for (const auto& loader : metadata.loaders) {
        std::string loaderPath;
        if (!loader.filePath.empty()) {
            loaderPath = (fs::path(packagePath) / loader.filePath).string();
        } else if (!loader.fileName.empty()) {
            loaderPath = (fs::path(packagePath) / "loaders" / loader.fileName).string();
        } else {
            report.warnings.push_back("Loader '" + loader.loaderId + "' has no file path specified");
            continue;
        }

        if (!checkFileExists(loaderPath)) {
            ValidationError err;
            err.field = "loader:" + loader.loaderId;
            err.message = "Loader file not found: " + loaderPath;
            err.error = DSPError::LoaderMissing;
            err.critical = true;
            report.errors.push_back(std::move(err));
            report.valid = false;
            continue;
        }
        report.loadersVerified++;

        if (!loader.expectedHash.empty()) {
            uint32_t expected = 0;
            for (size_t i = 0; i < loader.expectedHash.size(); ++i) {
                expected = (expected << 8) | loader.expectedHash[i];
            }
            if (!verifyChecksum(loaderPath, expected)) {
                ValidationError err;
                err.field = "loader:" + loader.loaderId;
                err.message = "Loader hash mismatch for: " + loader.loaderId;
                err.error = DSPError::ChecksumMismatch;
                err.critical = true;
                report.errors.push_back(std::move(err));
                report.valid = false;
            }
        }
    }

    report.info.push_back("Verified " + std::to_string(report.loadersVerified) + " loaders");
    return report;
}

ValidationReport DSPValidator::validateSchema(const std::string& packagePath) {
    ValidationReport report;
    report.valid = true;
    report.packageId = fs::path(packagePath).filename().string();

    static const std::vector<std::pair<std::string, std::string>> schemaFiles = {
        {"manifest.json", "manifest"},
        {"vendor.json", "vendor"},
    };

    for (const auto& [fileName, schemaName] : schemaFiles) {
        fs::path filePath = fs::path(packagePath) / fileName;
        if (!checkFileExists(filePath.string())) {
            report.info.push_back("Optional schema file not found: " + fileName);
            continue;
        }
        report.filesChecked++;

        if (!checkJsonSchema(filePath.string(), schemaName)) {
            ValidationError err;
            err.field = fileName;
            err.message = "Schema validation failed for: " + fileName;
            err.error = DSPError::ManifestInvalid;
            err.critical = true;
            report.errors.push_back(std::move(err));
            report.valid = false;
        } else {
            report.info.push_back("Schema valid: " + fileName);
        }
    }

    return report;
}

ValidationReport DSPValidator::validateDependencies(const DSPPackageMetadata& metadata,
    const std::vector<DSPPackageMetadata>& installed) {
    ValidationReport report;
    report.valid = true;
    report.packageId = metadata.manifest.packageId;
    report.packageVersion = metadata.manifest.version;

    std::unordered_set<std::string> installedIds;
    for (const auto& pkg : installed) {
        installedIds.insert(pkg.manifest.packageId);
    }

    for (const auto& dep : metadata.manifest.dependencies) {
        if (!dep.required) {
            report.info.push_back("Optional dependency '" + dep.packageId + "' not checked");
            continue;
        }

        if (installedIds.find(dep.packageId) == installedIds.end()) {
            ValidationError err;
            err.field = "dependency:" + dep.packageId;
            err.message = "Required dependency not installed: " + dep.packageId;
            err.error = DSPError::DependencyMissing;
            err.critical = true;
            report.errors.push_back(std::move(err));
            report.valid = false;
            continue;
        }

        for (const auto& installedPkg : installed) {
            if (installedPkg.manifest.packageId != dep.packageId) continue;

            if (!(dep.minVersion == DSPVersion{} && dep.maxVersion == DSPVersion{})) {
                const auto& iv = installedPkg.manifest.version;
                if (!(dep.minVersion == DSPVersion{}) && iv < dep.minVersion) {
                    ValidationError err;
                    err.field = "dependency:" + dep.packageId;
                    err.message = dep.packageId + " version " + iv.toString()
                        + " is below minimum " + dep.minVersion.toString();
                    err.error = DSPError::VersionMismatch;
                    err.critical = true;
                    report.errors.push_back(std::move(err));
                    report.valid = false;
                }
                if (!(dep.maxVersion == DSPVersion{}) && dep.maxVersion < iv) {
                    ValidationError err;
                    err.field = "dependency:" + dep.packageId;
                    err.message = dep.packageId + " version " + iv.toString()
                        + " exceeds maximum " + dep.maxVersion.toString();
                    err.error = DSPError::VersionMismatch;
                    err.critical = true;
                    report.errors.push_back(std::move(err));
                    report.valid = false;
                }
            }
            break;
        }
    }

    return report;
}

ValidationReport DSPValidator::validateSignature(const std::string& packagePath) {
    ValidationReport report;
    report.valid = false;
    report.packageId = fs::path(packagePath).filename().string();

    ValidationError err;
    err.field = "signature";
    err.message = "Signature verification not supported — no cryptographic backend configured";
    err.error = DSPError::SignatureInvalid;
    err.critical = false;
    report.errors.push_back(std::move(err));
    report.info.push_back("Signature verification: NotSupported — no cryptographic provider is available. DSP packages use checksum manifests for integrity verification, not cryptographic signatures.");
    return report;
}

ValidationReport DSPValidator::validateVersionCompatibility(const DSPPackageMetadata& metadata,
    const std::string& coreVersion) {
    ValidationReport report;
    report.valid = true;
    report.packageId = metadata.manifest.packageId;
    report.packageVersion = metadata.manifest.version;

    uint32_t coreMaj, coreMin, corePat;
    if (!versionStringToParts(coreVersion, coreMaj, coreMin, corePat)) {
        report.warnings.push_back("Could not parse core version: " + coreVersion);
        return report;
    }
    DSPVersion coreVer{coreMaj, coreMin, corePat};

    if (!metadata.manifest.minCoreVersion.empty()) {
        uint32_t minMaj, minMin, minPat;
        if (versionStringToParts(metadata.manifest.minCoreVersion, minMaj, minMin, minPat)) {
            DSPVersion minVer{minMaj, minMin, minPat};
            if (coreVer < minVer) {
                ValidationError err;
                err.field = "minCoreVersion";
                err.message = "Core version " + coreVersion + " is below minimum required "
                    + metadata.manifest.minCoreVersion;
                err.error = DSPError::VersionMismatch;
                err.critical = true;
                report.errors.push_back(std::move(err));
                report.valid = false;
            }
        } else {
            report.warnings.push_back("Could not parse minCoreVersion: " + metadata.manifest.minCoreVersion);
        }
    }

    if (!metadata.manifest.maxCoreVersion.empty()) {
        uint32_t maxMaj, maxMin, maxPat;
        if (versionStringToParts(metadata.manifest.maxCoreVersion, maxMaj, maxMin, maxPat)) {
            DSPVersion maxVer{maxMaj, maxMin, maxPat};
            if (maxVer < coreVer) {
                ValidationError err;
                err.field = "maxCoreVersion";
                err.message = "Core version " + coreVersion + " exceeds maximum supported "
                    + metadata.manifest.maxCoreVersion;
                err.error = DSPError::VersionMismatch;
                err.critical = true;
                report.errors.push_back(std::move(err));
                report.valid = false;
            }
        } else {
            report.warnings.push_back("Could not parse maxCoreVersion: " + metadata.manifest.maxCoreVersion);
        }
    }

    return report;
}

} // namespace dsp
} // namespace mbootcore
