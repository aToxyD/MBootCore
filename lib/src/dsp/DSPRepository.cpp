#include <mbootcore/dsp/DSPRepository.hpp>
#include <mbootcore/dsp/DSPManager.hpp>

#include "SafeParser.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <unordered_set>
#include <cctype>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

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

bool writeFileContents(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    return file.good();
}

std::vector<nlohmann::json> parseJsonObjects(const std::string& content) {
    try {
        auto j = nlohmann::json::parse(content);
        if (j.is_array()) return j.get<std::vector<nlohmann::json>>();
        if (j.is_object()) return {std::move(j)};
    } catch (...) {}
    try {
        std::vector<nlohmann::json> result;
        std::istringstream stream(content);
        nlohmann::json obj;
        while (stream >> obj) {
            result.push_back(std::move(obj));
        }
        if (!result.empty()) return result;
    } catch (...) {}
    return {};
}

// Version 1.0 of index format
constexpr const char* INDEX_FORMAT_VERSION = "1.0";

std::string vendorToString(discovery::Vendor v) {
    switch (v) {
        case discovery::Vendor::Qualcomm:   return "Qualcomm";
        case discovery::Vendor::MediaTek:   return "MediaTek";
        case discovery::Vendor::UNISOC:     return "UNISOC";
        case discovery::Vendor::Samsung:    return "Samsung";
        case discovery::Vendor::Rockchip:   return "Rockchip";
        case discovery::Vendor::Spreadtrum: return "Spreadtrum";
        case discovery::Vendor::Apple:      return "Apple";
        case discovery::Vendor::Google:     return "Google";
        case discovery::Vendor::Huawei:     return "Huawei";
        case discovery::Vendor::Custom:     return "Custom";
        default: return "Unknown";
    }
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

std::string protocolToString(discovery::ProtocolType p) {
    switch (p) {
        case discovery::ProtocolType::Sahara:        return "Sahara";
        case discovery::ProtocolType::Firehose:      return "Firehose";
        case discovery::ProtocolType::Fastboot:      return "Fastboot";
        case discovery::ProtocolType::MediaTekBROM:  return "MediaTekBROM";
        case discovery::ProtocolType::MediaTekDA:    return "MediaTekDA";
        case discovery::ProtocolType::UNISOCBootROM: return "UNISOCBootROM";
        case discovery::ProtocolType::UNISOCFDL:     return "UNISOCFDL";
        case discovery::ProtocolType::USBStream:     return "USBStream";
        case discovery::ProtocolType::Custom:        return "Custom";
        default: return "Unknown";
    }
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

std::string bootModeToString(discovery::BootMode b) {
    switch (b) {
        case discovery::BootMode::BootROM:      return "BootROM";
        case discovery::BootMode::EDL:          return "EDL";
        case discovery::BootMode::Firehose:     return "Firehose";
        case discovery::BootMode::Fastboot:     return "Fastboot";
        case discovery::BootMode::ADB:          return "ADB";
        case discovery::BootMode::Recovery:     return "Recovery";
        case discovery::BootMode::DownloadMode: return "DownloadMode";
        case discovery::BootMode::Preloader:    return "Preloader";
        case discovery::BootMode::BROM:         return "BROM";
        case discovery::BootMode::Custom:       return "Custom";
        default: return "Unknown";
    }
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



std::string dspOriginToString(DSPOrigin o) {
    switch (o) {
        case DSPOrigin::System:   return "system";
        case DSPOrigin::User:     return "user";
        case DSPOrigin::Portable: return "portable";
        case DSPOrigin::Online:   return "online";
        case DSPOrigin::Builtin:  return "builtin";
        default: return "unknown";
    }
}

std::string versionToString(const DSPVersion& v) {
    return std::to_string(v.major) + "." + std::to_string(v.minor) + "." + std::to_string(v.patch);
}

DSPVersion stringToVersion(const std::string& s) {
    DSPVersion v;
    std::regex ve(R"((\d+)\.(\d+)\.(\d+))");
    std::smatch m;
    if (std::regex_match(s, m, ve)) {
        auto r1 = fromCharsUint32(m[1].str());
        auto r2 = fromCharsUint32(m[2].str());
        auto r3 = fromCharsUint32(m[3].str());
        if (r1.ok) v.major = r1.value;
        if (r2.ok) v.minor = r2.value;
        if (r3.ok) v.patch = r3.value;
    }
    return v;
}

std::string timePointToString(const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
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

DSPPackageStatistics buildStatisticsFromMetadata(const DSPPackageMetadata& meta) {
    DSPPackageStatistics stats;
    stats.packageId = meta.manifest.packageId;
    stats.name = meta.manifest.name;
    stats.version = meta.manifest.version;
    stats.state = meta.state;
    stats.origin = meta.origin;
    stats.loaderCount = static_cast<uint32_t>(meta.loaders.size());
    stats.chipsetCount = static_cast<uint32_t>(meta.chipsets.size());
    stats.profileCount = static_cast<uint32_t>(meta.profiles.size());
    stats.quirkCount = static_cast<uint32_t>(meta.quirks.size());
    stats.compressedSize = meta.manifest.fileSize;
    stats.installDate = meta.installDate;
    stats.tags = meta.manifest.tags;

    if (!meta.installPath.empty()) {
        std::error_code ec;
        uint64_t total = 0;
        for (const auto& entry : fs::recursive_directory_iterator(meta.installPath, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) total += entry.file_size();
        }
        if (!ec) stats.installedSize = total;
    }

    return stats;
}

bool caseInsensitiveCompare(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

bool containsText(const std::string& text, const std::string& query, bool caseSensitive) {
    if (text.empty() || query.empty()) return true;
    if (caseSensitive) return text.find(query) != std::string::npos;
    if (text.size() < query.size()) return false;
    for (size_t i = 0; i <= text.size() - query.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < query.size(); ++j) {
            if (std::tolower(static_cast<unsigned char>(text[i + j])) !=
                std::tolower(static_cast<unsigned char>(query[j]))) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

} // anonymous namespace

struct DSPRepository::Impl {
    std::string m_systemPath;
    std::string m_userPath;
    std::string m_portablePath;

    std::unordered_map<std::string, DSPPackageMetadata> m_packages;
    std::vector<DSPPackageStatistics> m_index;

    // Indexed lookup helpers
    std::unordered_map<std::string, std::vector<const DSPPackageMetadata*>> m_byVendor;
    std::unordered_map<std::string, std::vector<const DSPChipsetMetadata*>> m_allChipsets;
    std::unordered_map<std::string, std::vector<const DSPLoaderMetadata*>> m_allLoaders;
    std::vector<const DSPBootModeMetadata*> m_allBootModes;
    std::vector<const DSPTransportMetadata*> m_allTransports;
    std::vector<const DSPProtocolMetadata*> m_allProtocols;
    std::vector<const DSPVendorMetadata*> m_allVendors;
    std::unordered_map<std::string, std::vector<const DSPPackageMetadata*>> m_byProtocol;
    std::unordered_map<std::string, std::vector<const DSPPackageMetadata*>> m_byBootMode;
    std::unordered_map<std::string, std::vector<const DSPPackageMetadata*>> m_byLoader;
    std::unordered_map<std::string, std::vector<const DSPPackageMetadata*>> m_byOrigin;

    void rebuildIndex();
    void scanDirectory(const std::string& path);
    Result<DSPPackageMetadata> loadPackage(const std::string& dirPath);
    bool matchesQuery(const DSPPackageMetadata& pkg, const DSPQuery& query) const;
};

DSPRepository::DSPRepository() : m_impl(std::make_unique<Impl>()) {}
DSPRepository::~DSPRepository() = default;

void DSPRepository::setSystemPath(const std::string& path) { m_impl->m_systemPath = path; }
void DSPRepository::setUserPath(const std::string& path) { m_impl->m_userPath = path; }
void DSPRepository::setPortablePath(const std::string& path) { m_impl->m_portablePath = path; }
std::string DSPRepository::systemPath() const noexcept { return m_impl->m_systemPath; }
std::string DSPRepository::userPath() const noexcept { return m_impl->m_userPath; }
std::string DSPRepository::portablePath() const noexcept { return m_impl->m_portablePath; }

Result<void> DSPRepository::scan() {
    m_impl->m_packages.clear();
    if (!m_impl->m_systemPath.empty()) {
        m_impl->scanDirectory(m_impl->m_systemPath);
    }
    if (!m_impl->m_userPath.empty()) {
        m_impl->scanDirectory(m_impl->m_userPath);
    }
    if (!m_impl->m_portablePath.empty()) {
        m_impl->scanDirectory(m_impl->m_portablePath);
    }
    m_impl->rebuildIndex();
    return {};
}

Result<void> DSPRepository::scanPath(const std::string& path) {
    m_impl->scanDirectory(path);
    m_impl->rebuildIndex();
    return {};
}

std::vector<DSPPackageStatistics> DSPRepository::listAll() const {
    std::vector<DSPPackageStatistics> result;
    result.reserve(m_impl->m_index.size());
    for (const auto& stats : m_impl->m_index) {
        result.push_back(stats);
    }
    return result;
}

std::vector<DSPPackageStatistics> DSPRepository::search(const DSPQuery& query) const {
    std::vector<DSPPackageStatistics> result;
    for (const auto& [id, pkg] : m_impl->m_packages) {
        if (m_impl->matchesQuery(pkg, query)) {
            result.push_back(buildStatisticsFromMetadata(pkg));
        }
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.packageId < b.packageId;
    });
    return result;
}

const DSPPackageMetadata* DSPRepository::byId(const std::string& packageId) const {
    auto it = m_impl->m_packages.find(packageId);
    if (it != m_impl->m_packages.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const DSPPackageMetadata*> DSPRepository::byVendor(discovery::Vendor vendor) const {
    auto key = vendorToString(vendor);
    auto it = m_impl->m_byVendor.find(key);
    if (it != m_impl->m_byVendor.end()) return it->second;
    return {};
}

std::vector<const DSPPackageMetadata*> DSPRepository::byChipset(const ChipsetId& id) const {
    std::vector<const DSPPackageMetadata*> result;
    std::string targetKey = id.toString();
    for (const auto& [pkgId, pkg] : m_impl->m_packages) {
        for (const auto& cs : pkg.chipsets) {
            if (cs.id.toString() == targetKey) {
                result.push_back(&pkg);
                break;
            }
        }
    }
    return result;
}

std::vector<const DSPPackageMetadata*> DSPRepository::byProtocol(discovery::ProtocolType proto) const {
    auto key = protocolToString(proto);
    auto it = m_impl->m_byProtocol.find(key);
    if (it != m_impl->m_byProtocol.end()) return it->second;
    return {};
}

std::vector<const DSPPackageMetadata*> DSPRepository::byBootMode(discovery::BootMode mode) const {
    auto key = bootModeToString(mode);
    auto it = m_impl->m_byBootMode.find(key);
    if (it != m_impl->m_byBootMode.end()) return it->second;
    return {};
}

std::vector<const DSPPackageMetadata*> DSPRepository::byLoader(const std::string& loaderName) const {
    auto it = m_impl->m_byLoader.find(loaderName);
    if (it != m_impl->m_byLoader.end()) return it->second;
    return {};
}

std::vector<const DSPPackageMetadata*> DSPRepository::byOrigin(DSPOrigin origin) const {
    auto key = dspOriginToString(origin);
    auto it = m_impl->m_byOrigin.find(key);
    if (it != m_impl->m_byOrigin.end()) return it->second;
    return {};
}

std::vector<const DSPChipsetMetadata*> DSPRepository::allChipsets() const {
    std::vector<const DSPChipsetMetadata*> result;
    for (const auto& [_, vec] : m_impl->m_allChipsets) {
        for (auto* cs : vec) {
            result.push_back(cs);
        }
    }
    return result;
}

std::vector<const DSPLoaderMetadata*> DSPRepository::allLoaders() const {
    std::vector<const DSPLoaderMetadata*> result;
    for (const auto& [_, vec] : m_impl->m_allLoaders) {
        for (auto* ld : vec) {
            result.push_back(ld);
        }
    }
    return result;
}

std::vector<const DSPBootModeMetadata*> DSPRepository::allBootModes() const {
    return m_impl->m_allBootModes;
}

std::vector<const DSPTransportMetadata*> DSPRepository::allTransports() const {
    return m_impl->m_allTransports;
}

std::vector<const DSPProtocolMetadata*> DSPRepository::allProtocols() const {
    return m_impl->m_allProtocols;
}

std::vector<const DSPVendorMetadata*> DSPRepository::allVendors() const {
    return m_impl->m_allVendors;
}

size_t DSPRepository::packageCount() const noexcept {
    return m_impl->m_packages.size();
}

bool DSPRepository::isEmpty() const noexcept {
    return m_impl->m_packages.empty();
}

void DSPRepository::clear() {
    m_impl->m_packages.clear();
    m_impl->m_index.clear();
    m_impl->m_byVendor.clear();
    m_impl->m_allChipsets.clear();
    m_impl->m_allLoaders.clear();
    m_impl->m_allBootModes.clear();
    m_impl->m_allTransports.clear();
    m_impl->m_allProtocols.clear();
    m_impl->m_allVendors.clear();
    m_impl->m_byProtocol.clear();
    m_impl->m_byBootMode.clear();
    m_impl->m_byLoader.clear();
    m_impl->m_byOrigin.clear();
}

Result<void> DSPRepository::saveIndex() {
    std::string indexDir;
    if (!m_impl->m_userPath.empty()) indexDir = m_impl->m_userPath;
    else if (!m_impl->m_systemPath.empty()) indexDir = m_impl->m_systemPath;
    else if (!m_impl->m_portablePath.empty()) indexDir = m_impl->m_portablePath;
    else return static_cast<ErrorCode>(DSPError::RepositoryUnavailable);

    auto indexPath = (fs::path(indexDir) / "index.json").string();

    try {
        nlohmann::json j;
        j["formatVersion"] = INDEX_FORMAT_VERSION;
        j["packageCount"] = m_impl->m_packages.size();

        nlohmann::json packages = nlohmann::json::array();
        for (const auto& [id, pkg] : m_impl->m_packages) {
            nlohmann::json p;
            p["packageId"] = pkg.manifest.packageId;
            p["name"] = pkg.manifest.name;
            p["version"] = versionToString(pkg.manifest.version);
            p["vendor"] = pkg.manifest.vendor;
            p["state"] = dspStateToString(pkg.state);
            p["origin"] = dspOriginToString(pkg.origin);
            p["installPath"] = pkg.installPath;
            p["loaderCount"] = pkg.loaders.size();
            p["chipsetCount"] = pkg.chipsets.size();
            p["installDate"] = timePointToString(pkg.installDate);
            packages.push_back(std::move(p));
        }
        j["packages"] = std::move(packages);

        if (!writeFileContents(indexPath, j.dump(2))) {
            return static_cast<ErrorCode>(DSPError::DatabaseError);
        }
    } catch (...) {
        return static_cast<ErrorCode>(DSPError::DatabaseError);
    }

    return {};
}

Result<void> DSPRepository::loadIndex() {
    std::string indexDir;
    if (!m_impl->m_userPath.empty()) {
        indexDir = m_impl->m_userPath;
    } else if (!m_impl->m_systemPath.empty()) {
        indexDir = m_impl->m_systemPath;
    } else if (!m_impl->m_portablePath.empty()) {
        indexDir = m_impl->m_portablePath;
    } else {
        return static_cast<ErrorCode>(DSPError::RepositoryUnavailable);
    }

    auto indexPath = (fs::path(indexDir) / "index.json").string();
    if (!fs::exists(indexPath)) {
        return static_cast<ErrorCode>(DSPError::RepositoryUnavailable);
    }

    auto content = readFileContents(indexPath);
    if (content.empty()) {
        return static_cast<ErrorCode>(DSPError::DatabaseCorrupted);
    }

    nlohmann::json parser;
    try {
        parser = nlohmann::json::parse(content);
    } catch (...) {
        return static_cast<ErrorCode>(DSPError::DatabaseCorrupted);
    }
    if (!parser.is_object()) {
        return static_cast<ErrorCode>(DSPError::DatabaseCorrupted);
    }

    auto fmtVer = parser.value("formatVersion", std::string{});
    if (fmtVer.empty()) {
        return static_cast<ErrorCode>(DSPError::DatabaseCorrupted);
    }

    return {};
}

void DSPRepository::Impl::scanDirectory(const std::string& path) {
    if (!fs::exists(path) || !fs::is_directory(path)) return;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        // Check if directory contains manifest.json
        auto manifestPath = entry.path() / "manifest.json";
        if (!fs::exists(manifestPath)) continue;

        auto result = loadPackage(entry.path().string());
        if (result.isOk()) {
            auto& pkg = result.value();
            auto pkgId = pkg.manifest.packageId;
            if (!pkgId.empty()) {
                m_packages[pkgId] = std::move(pkg);
            }
        }
    }
}

Result<DSPPackageMetadata> DSPRepository::Impl::loadPackage(const std::string& dirPath) {
    DSPPackageMetadata meta;

    auto manifestPath = (fs::path(dirPath) / "manifest.json").string();
    if (!fs::exists(manifestPath)) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    auto manifestContent = readFileContents(manifestPath);
    if (manifestContent.empty()) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    nlohmann::json mj;
    try {
        mj = nlohmann::json::parse(manifestContent);
    } catch (...) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }
    if (!mj.is_object()) {
        return static_cast<ErrorCode>(DSPError::ManifestInvalid);
    }

    auto jStr = [](const nlohmann::json& j, const std::string& k) -> std::string {
        auto it = j.find(k);
        return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string{};
    };
    auto jStrArr = [](const nlohmann::json& j, const std::string& k) -> std::vector<std::string> {
        std::vector<std::string> r;
        auto it = j.find(k);
        if (it != j.end() && it->is_array()) {
            for (const auto& e : *it) {
                if (e.is_string()) r.push_back(e.get<std::string>());
            }
        }
        return r;
    };
    auto jObjEntries = [](const nlohmann::json& j, const std::string& k) -> std::vector<std::pair<std::string, std::string>> {
        std::vector<std::pair<std::string, std::string>> r;
        auto it = j.find(k);
        if (it != j.end() && it->is_object()) {
            for (auto [ok, ov] : it->items()) {
                r.emplace_back(ok, ov.is_string() ? ov.get<std::string>() : std::string{});
            }
        }
        return r;
    };
    auto jUint = [](const nlohmann::json& j, const std::string& k) -> uint64_t {
        auto it = j.find(k);
        return (it != j.end() && it->is_number_unsigned()) ? it->get<uint64_t>() : 0;
    };
    auto jBool = [](const nlohmann::json& j, const std::string& k) -> bool {
        auto it = j.find(k);
        return (it != j.end() && it->is_boolean()) ? it->get<bool>() : false;
    };

    // Parse basic manifest fields
    meta.manifest.formatVersion = jStr(mj, "formatVersion");
    meta.manifest.packageId = jStr(mj, "packageId");
    meta.manifest.name = jStr(mj, "name");
    meta.manifest.vendor = jStr(mj, "vendor");
    meta.manifest.description = jStr(mj, "description");
    meta.manifest.license = jStr(mj, "license");
    meta.manifest.minCoreVersion = jStr(mj, "minCoreVersion");
    meta.manifest.maxCoreVersion = jStr(mj, "maxCoreVersion");
    meta.manifest.checksumAlgorithm = jStr(mj, "checksumAlgorithm");
    meta.manifest.signatureAlgorithm = jStr(mj, "signatureAlgorithm");

    meta.manifest.fileSize = jUint(mj, "fileSize");
    meta.manifest.checksum = static_cast<uint32_t>(jUint(mj, "checksum"));
    meta.manifest.requiresSignature = jBool(mj, "requiresSignature");
    meta.manifest.compressLoaders = jBool(mj, "compressLoaders");

    meta.manifest.tags = jStrArr(mj, "tags");
    meta.manifest.authors = jStrArr(mj, "authors");

    auto versionStr = jStr(mj, "version");
    if (!versionStr.empty()) {
        meta.manifest.version = stringToVersion(versionStr);
    }

    // Dependencies
    auto depEntries = jObjEntries(mj, "dependencies");
    for (const auto& [depId, depVer] : depEntries) {
        DSPDependency dep;
        dep.packageId = depId;
        dep.name = depId;
        dep.required = true;
        if (!depVer.empty()) {
            std::regex verRange(R"((\d+\.\d+\.\d+)\s*-\s*(\d+\.\d+\.\d+))");
            std::smatch m;
            if (std::regex_match(depVer, m, verRange)) {
                dep.minVersion = stringToVersion(m[1]);
                dep.maxVersion = stringToVersion(m[2]);
            } else {
                dep.minVersion = stringToVersion(depVer);
                dep.maxVersion = dep.minVersion;
            }
        }
        meta.manifest.dependencies.push_back(std::move(dep));
    }

    auto buildDate = jStr(mj, "buildDate");
    if (!buildDate.empty()) meta.manifest.buildDate = stringToTimePoint(buildDate);
    auto releaseDate = jStr(mj, "releaseDate");
    if (!releaseDate.empty()) meta.manifest.releaseDate = stringToTimePoint(releaseDate);

    // Vendor metadata
    auto vendorPath = (fs::path(dirPath) / "vendor.json").string();
    if (fs::exists(vendorPath)) {
        auto vContent = readFileContents(vendorPath);
        if (!vContent.empty()) {
            try {
                auto vj = nlohmann::json::parse(vContent);
                if (vj.is_object()) {
                    meta.vendor.vendorName = jStr(vj, "vendorName");
                    meta.vendor.displayName = jStr(vj, "displayName");
                    meta.vendor.website = jStr(vj, "website");
                    meta.vendor.defaultProtocol = jStr(vj, "defaultProtocol");
                    meta.vendor.defaultBootMode = jStr(vj, "defaultBootMode");
                    meta.vendor.defaultTransport = jStr(vj, "defaultTransport");
                    meta.vendor.aliases = jStrArr(vj, "aliases");
                    meta.vendor.supportedChipsets = jStrArr(vj, "supportedChipsets");
                    meta.vendor.supportedBootModes = jStrArr(vj, "supportedBootModes");
                    meta.vendor.supportedProtocols = jStrArr(vj, "supportedProtocols");
                    meta.vendor.supportedTransports = jStrArr(vj, "supportedTransports");

                    auto vid = jStr(vj, "vendorId");
                    if (!vid.empty()) meta.vendor.vendorId = stringToVendor(vid);

                    auto props = jObjEntries(vj, "properties");
                    for (const auto& [k, v] : props) meta.vendor.properties[k] = v;
                }
            } catch (...) {}
        }
    }

    // Chipset metadata
    auto chipsetsPath = (fs::path(dirPath) / "chipsets.json").string();
    if (fs::exists(chipsetsPath)) {
        auto csContent = readFileContents(chipsetsPath);
        if (!csContent.empty()) {
            auto csObjs = parseJsonObjects(csContent);
            for (const auto& cj : csObjs) {
                DSPChipsetMetadata cs;
                cs.id.vendor = jStr(cj, "vendor");
                cs.id.family = jStr(cj, "family");
                cs.id.variant = jStr(cj, "variant");
                cs.displayName = jStr(cj, "displayName");
                cs.manufacturer = jStr(cj, "manufacturer");
                cs.architecture = jStr(cj, "architecture");
                cs.defaultProgrammer = jStr(cj, "defaultProgrammer");
                cs.maxMemoryMB = static_cast<uint32_t>(jUint(cj, "maxMemoryMB"));
                cs.maxFlashSizeMB = static_cast<uint32_t>(jUint(cj, "maxFlashSizeMB"));
                cs.hasTrustZone = jBool(cj, "hasTrustZone");
                cs.hasSecureBoot = jBool(cj, "hasSecureBoot");
                cs.hasDebugPort = jBool(cj, "hasDebugPort");
                cs.variants = jStrArr(cj, "variants");
                cs.families = jStrArr(cj, "families");
                cs.predecessors = jStrArr(cj, "predecessors");
                cs.successors = jStrArr(cj, "successors");
                cs.compatibleLoaders = jStrArr(cj, "compatibleLoaders");
                cs.knownIssues = jStrArr(cj, "knownIssues");

                for (const auto& bm : jStrArr(cj, "supportedBootModes"))
                    cs.supportedBootModes.push_back(stringToBootMode(bm));
                for (const auto& pr : jStrArr(cj, "supportedProtocols"))
                    cs.supportedProtocols.push_back(stringToProtocol(pr));
                for (const auto& st : jStrArr(cj, "supportedStorage"))
                    cs.supportedStorage.push_back(stringToStorageType(st));

                auto relDate = jStr(cj, "releaseDate");
                if (!relDate.empty()) cs.releaseDate = stringToTimePoint(relDate);

                auto props = jObjEntries(cj, "properties");
                for (const auto& [k, v] : props) cs.properties[k] = v;

                meta.chipsets.push_back(std::move(cs));
            }
        }
    }

    // Loader metadata from loaders.json and loaders directory
    auto loadersJsonPath = (fs::path(dirPath) / "loaders.json").string();
    if (fs::exists(loadersJsonPath)) {
        auto ldContent = readFileContents(loadersJsonPath);
        if (!ldContent.empty()) {
            auto ldObjs = parseJsonObjects(ldContent);
            for (const auto& lj : ldObjs) {
                DSPLoaderMetadata ld;
                ld.loaderId = jStr(lj, "loaderId");
                ld.name = jStr(lj, "name");
                ld.fileName = jStr(lj, "fileName");
                ld.filePath = jStr(lj, "filePath");
                ld.description = jStr(lj, "description");
                ld.hashAlgorithm = jStr(lj, "hashAlgorithm");
                ld.version = stringToVersion(
                    std::to_string(jUint(lj, "versionMajor")) + "." +
                    std::to_string(jUint(lj, "versionMinor")) + "." +
                    std::to_string(jUint(lj, "versionPatch")));
                ld.priority = static_cast<uint32_t>(jUint(lj, "priority"));
                ld.originalSize = jUint(lj, "originalSize");
                ld.compressedSize = jUint(lj, "compressedSize");
                ld.isCompressed = jBool(lj, "isCompressed");
                ld.isFallback = jBool(lj, "isFallback");
                ld.isPreferred = jBool(lj, "isPreferred");
                ld.isSigned = jBool(lj, "isSigned");
                ld.requiresAuthentication = jBool(lj, "requiresAuthentication");
                ld.tags = jStrArr(lj, "tags");
                ld.compatibleVendors = jStrArr(lj, "compatibleVendors");

                for (const auto& pr : jStrArr(lj, "protocols"))
                    ld.protocols.push_back(stringToProtocol(pr));
                for (const auto& bm : jStrArr(lj, "bootModes"))
                    ld.bootModes.push_back(stringToBootMode(bm));

                auto rbm = jStr(lj, "requiredBootMode");
                if (!rbm.empty()) ld.requiredBootMode = stringToBootMode(rbm);

                // Compatible chipsets (array of objects)
                auto ccIt = lj.find("compatibleChipsets");
                if (ccIt != lj.end() && ccIt->is_array()) {
                    for (const auto& ce : *ccIt) {
                        if (!ce.is_object()) continue;
                        ChipsetId cid;
                        cid.vendor = jStr(ce, "vendor");
                        cid.family = jStr(ce, "family");
                        cid.variant = jStr(ce, "variant");
                        if (!cid.vendor.empty() && !cid.family.empty())
                            ld.compatibleChipsets.push_back(cid);
                    }
                }

                auto hashStr = jStr(lj, "expectedHash");
                if (!hashStr.empty()) ld.expectedHash.assign(hashStr.begin(), hashStr.end());

                auto props = jObjEntries(lj, "properties");
                for (const auto& [k, v] : props) ld.properties[k] = v;

                // Resolve file path
                if (ld.filePath.empty() && !ld.fileName.empty()) {
                    auto lf = fs::path(dirPath) / "loaders" / ld.fileName;
                    if (fs::exists(lf)) ld.filePath = lf.string();
                }

                meta.loaders.push_back(std::move(ld));
            }
        }
    }

    // Auto-detect loader files
    auto loadersDir = fs::path(dirPath) / "loaders";
    if (fs::exists(loadersDir) && fs::is_directory(loadersDir)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(loadersDir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            auto fname = entry.path().filename().string();
            bool found = false;
            for (const auto& ld : meta.loaders) {
                if (ld.fileName == fname || ld.filePath == entry.path().string()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                DSPLoaderMetadata ald;
                ald.loaderId = fname;
                ald.name = entry.path().stem().string();
                ald.fileName = fname;
                ald.filePath = entry.path().string();
                ald.originalSize = entry.file_size();
                ald.version.major = 1;
                meta.loaders.push_back(std::move(ald));
            }
        }
    }

    // Boot modes
    auto bmPath = (fs::path(dirPath) / "bootmodes.json").string();
    if (fs::exists(bmPath)) {
        auto bmContent = readFileContents(bmPath);
        if (!bmContent.empty()) {
            auto bmObjs = parseJsonObjects(bmContent);
            for (const auto& bj : bmObjs) {
                DSPBootModeMetadata bm;
                bm.bootModeId = jStr(bj, "bootModeId");
                bm.displayName = jStr(bj, "displayName");
                bm.description = jStr(bj, "description");
                bm.defaultTimeoutMs = static_cast<uint32_t>(jUint(bj, "defaultTimeoutMs"));
                bm.requiresAuth = jBool(bj, "requiresAuth");
                bm.requireUsb = jBool(bj, "requireUsb");
                bm.requireSerial = jBool(bj, "requireSerial");
                bm.requireTcp = jBool(bj, "requireTcp");
                bm.requiredKeys = jStrArr(bj, "requiredKeys");
                bm.quirks = jStrArr(bj, "quirks");

                auto bmStr = jStr(bj, "bootMode");
                if (!bmStr.empty()) bm.bootMode = stringToBootMode(bmStr);

                for (const auto& p : jStrArr(bj, "supportedProtocols"))
                    bm.supportedProtocols.push_back(stringToProtocol(p));

                auto props = jObjEntries(bj, "properties");
                for (const auto& [k, v] : props) bm.properties[k] = v;

                meta.bootModes.push_back(std::move(bm));
            }
        }
    }

    // Transports
    auto tpPath = (fs::path(dirPath) / "transports.json").string();
    if (fs::exists(tpPath)) {
        auto tpContent = readFileContents(tpPath);
        if (!tpContent.empty()) {
            auto tpObjs = parseJsonObjects(tpContent);
            for (const auto& tj : tpObjs) {
                DSPTransportMetadata tp;
                tp.transportId = jStr(tj, "transportId");
                tp.displayName = jStr(tj, "displayName");
                tp.description = jStr(tj, "description");
                tp.defaultBaudRate = static_cast<uint32_t>(jUint(tj, "defaultBaudRate"));
                tp.defaultTimeoutMs = static_cast<uint32_t>(jUint(tj, "defaultTimeoutMs"));
                tp.maxPacketSize = static_cast<uint32_t>(jUint(tj, "maxPacketSize"));
                tp.supportsHotplug = jBool(tj, "supportsHotplug");
                tp.supportsAsync = jBool(tj, "supportsAsync");

                for (const auto& br : jStrArr(tj, "supportedBaudRates")) {
                    auto r = fromCharsUint32(br);
                    if (r.ok) tp.supportedBaudRates.push_back(r.value);
                }

                auto tpType = jStr(tj, "transportType");
                if (tpType == "USB") tp.transportType = discovery::TransportType::USB;
                else if (tpType == "Serial") tp.transportType = discovery::TransportType::Serial;
                else if (tpType == "TCP") tp.transportType = discovery::TransportType::TCP;
                else if (tpType == "Virtual") tp.transportType = discovery::TransportType::Virtual;

                auto props = jObjEntries(tj, "properties");
                for (const auto& [k, v] : props) tp.properties[k] = v;

                meta.transports.push_back(std::move(tp));
            }
        }
    }

    // Protocols
    auto prPath = (fs::path(dirPath) / "protocols.json").string();
    if (fs::exists(prPath)) {
        auto prContent = readFileContents(prPath);
        if (!prContent.empty()) {
            auto prObjs = parseJsonObjects(prContent);
            for (const auto& pj : prObjs) {
                DSPProtocolMetadata pr;
                pr.protocolId = jStr(pj, "protocolId");
                pr.displayName = jStr(pj, "displayName");
                pr.description = jStr(pj, "description");
                pr.defaultTimeoutMs = static_cast<uint32_t>(jUint(pj, "defaultTimeoutMs"));
                pr.requiresProgrammer = jBool(pj, "requiresProgrammer");
                pr.supportsBackup = jBool(pj, "supportsBackup");
                pr.supportsVerify = jBool(pj, "supportsVerify");
                pr.supportsResume = jBool(pj, "supportsResume");
                pr.requiredLoaders = jStrArr(pj, "requiredLoaders");
                pr.optionalLoaders = jStrArr(pj, "optionalLoaders");
                pr.quirks = jStrArr(pj, "quirks");

                auto minVer = jStr(pj, "minVersion");
                if (!minVer.empty()) pr.minVersion = stringToVersion(minVer);
                auto maxVer = jStr(pj, "maxVersion");
                if (!maxVer.empty()) pr.maxVersion = stringToVersion(maxVer);

                auto prType = jStr(pj, "protocolType");
                if (!prType.empty()) pr.protocolType = stringToProtocol(prType);

                for (const auto& b : jStrArr(pj, "supportedBootModes"))
                    pr.supportedBootModes.push_back(stringToBootMode(b));

                auto props = jObjEntries(pj, "properties");
                for (const auto& [k, v] : props) pr.properties[k] = v;

                meta.protocols.push_back(std::move(pr));
            }
        }
    }

    // Profiles
    auto profilesDir = fs::path(dirPath) / "profiles";
    if (fs::exists(profilesDir) && fs::is_directory(profilesDir)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(profilesDir, ec)) {
            if (ec) break;
            if (entry.is_regular_file()) {
                meta.profiles.push_back(entry.path().filename().string());
            }
        }
    }

    // State and origin
    meta.state = DSPState::Installed;
    meta.origin = DSPOrigin::System;
    meta.installPath = dirPath;
    meta.installDate = std::chrono::system_clock::now();

    // Determine origin based on path
    if (!m_userPath.empty() && dirPath.find(m_userPath) == 0) {
        meta.origin = DSPOrigin::User;
    } else if (!m_systemPath.empty() && dirPath.find(m_systemPath) == 0) {
        meta.origin = DSPOrigin::System;
    } else if (!m_portablePath.empty() && dirPath.find(m_portablePath) == 0) {
        meta.origin = DSPOrigin::Portable;
    }

    return meta;
}

void DSPRepository::Impl::rebuildIndex() {
    m_index.clear();
    m_byVendor.clear();
    m_allChipsets.clear();
    m_allLoaders.clear();
    m_allBootModes.clear();
    m_allTransports.clear();
    m_allProtocols.clear();
    m_allVendors.clear();
    m_byProtocol.clear();
    m_byBootMode.clear();
    m_byLoader.clear();
    m_byOrigin.clear();

    m_index.reserve(m_packages.size());

    for (const auto& [id, pkg] : m_packages) {
        m_index.push_back(buildStatisticsFromMetadata(pkg));

        // By vendor
        auto vendorKey = pkg.manifest.vendor.empty() ? "Unknown" : pkg.manifest.vendor;
        m_byVendor[vendorKey].push_back(&pkg);

        // By protocol
        for (const auto& pr : pkg.protocols) {
            auto key = protocolToString(pr.protocolType);
            m_byProtocol[key].push_back(&pkg);
        }

        // By boot mode
        for (const auto& bm : pkg.bootModes) {
            auto key = bootModeToString(bm.bootMode);
            m_byBootMode[key].push_back(&pkg);
        }

        // By loader
        for (const auto& ld : pkg.loaders) {
            m_allLoaders[ld.loaderId].push_back(&ld);
            if (!ld.name.empty()) {
                m_byLoader[ld.name].push_back(&pkg);
            }
            // Also index by compatible vendors
            for (const auto& cv : ld.compatibleVendors) {
                m_byLoader[cv].push_back(&pkg);
            }
        }

        // Chipsets
        for (const auto& cs : pkg.chipsets) {
            auto key = cs.id.toString();
            m_allChipsets[key].push_back(&cs);
        }

        // Boot modes
        for (const auto& bm : pkg.bootModes) {
            m_allBootModes.push_back(&bm);
        }

        // Transports
        for (const auto& tp : pkg.transports) {
            m_allTransports.push_back(&tp);
        }

        // Protocols
        for (const auto& pr : pkg.protocols) {
            m_allProtocols.push_back(&pr);
        }

        // By origin
        auto originKey = dspOriginToString(pkg.origin);
        m_byOrigin[originKey].push_back(&pkg);

        // Vendor metadata
        if (!pkg.vendor.vendorName.empty()) {
            m_allVendors.push_back(&pkg.vendor);
        }
    }
}

bool DSPRepository::Impl::matchesQuery(const DSPPackageMetadata& pkg, const DSPQuery& query) const {
    // Search text
    if (!query.searchText.empty()) {
        bool textMatch = false;
        if (containsText(pkg.manifest.packageId, query.searchText, query.caseSensitive))
            textMatch = true;
        if (containsText(pkg.manifest.name, query.searchText, query.caseSensitive))
            textMatch = true;
        if (containsText(pkg.manifest.description, query.searchText, query.caseSensitive))
            textMatch = true;
        if (containsText(pkg.manifest.vendor, query.searchText, query.caseSensitive))
            textMatch = true;
        if (!textMatch) return false;
    }

    // Vendor filter
    if (query.vendor != discovery::Vendor::Unknown) {
        auto vendorStr = vendorToString(query.vendor);
        if (!query.caseSensitive) {
            if (!caseInsensitiveCompare(pkg.manifest.vendor, vendorStr)) return false;
        } else {
            if (pkg.manifest.vendor != vendorStr) return false;
        }
    }

    // Chipset filter
    if (!query.chipset.vendor.empty() || !query.chipset.family.empty()) {
        bool chipsetMatch = false;
        for (const auto& cs : pkg.chipsets) {
            bool cv = query.chipset.vendor.empty() ||
                (query.caseSensitive ? cs.id.vendor == query.chipset.vendor
                                     : caseInsensitiveCompare(cs.id.vendor, query.chipset.vendor));
            bool cf = query.chipset.family.empty() ||
                (query.caseSensitive ? cs.id.family == query.chipset.family
                                     : caseInsensitiveCompare(cs.id.family, query.chipset.family));
            if (cv && cf) { chipsetMatch = true; break; }
        }
        if (!chipsetMatch) return false;
    }

    // Loader name filter
    if (!query.loaderName.empty()) {
        bool loaderMatch = false;
        for (const auto& ld : pkg.loaders) {
            bool nameOk = query.caseSensitive ? ld.loaderId == query.loaderName
                                              : caseInsensitiveCompare(ld.loaderId, query.loaderName);
            bool displayOk = query.caseSensitive ? ld.name == query.loaderName
                                                 : caseInsensitiveCompare(ld.name, query.loaderName);
            if (nameOk || displayOk) { loaderMatch = true; break; }
        }
        if (!loaderMatch) return false;
    }

    // Boot mode filter
    if (query.bootMode != discovery::BootMode::Unknown) {
        bool bmMatch = false;
        for (const auto& bm : pkg.bootModes) {
            if (bm.bootMode == query.bootMode) { bmMatch = true; break; }
        }
        if (!bmMatch) return false;
    }

    // Protocol filter
    if (query.protocol != discovery::ProtocolType::Unknown) {
        bool prMatch = false;
        for (const auto& pr : pkg.protocols) {
            if (pr.protocolType == query.protocol) { prMatch = true; break; }
        }
        if (!prMatch) return false;
    }

    // Version range
    if (query.minVersion.major > 0 || query.minVersion.minor > 0 || query.minVersion.patch > 0) {
        if (pkg.manifest.version < query.minVersion) return false;
    }
    if (query.maxVersion.major > 0 || query.maxVersion.minor > 0 || query.maxVersion.patch > 0) {
        if (query.maxVersion < pkg.manifest.version) return false;
    }

    // Capability filter
    if (!query.capability.empty()) {
        // Check protocol capabilities and vendor properties
        bool capMatch = false;
        for (const auto& pr : pkg.protocols) {
            for (const auto& [k, v] : pr.properties) {
                if (containsText(k, query.capability, query.caseSensitive) ||
                    containsText(v, query.capability, query.caseSensitive)) {
                    capMatch = true; break;
                }
            }
            if (capMatch) break;
        }
        if (!capMatch) {
            for (const auto& [k, v] : pkg.vendor.properties) {
                if (containsText(k, query.capability, query.caseSensitive) ||
                    containsText(v, query.capability, query.caseSensitive)) {
                    capMatch = true; break;
                }
            }
        }
        if (!capMatch) return false;
    }

    // Tag filter
    if (!query.tag.empty()) {
        bool tagMatch = false;
        for (const auto& t : pkg.manifest.tags) {
            if (query.caseSensitive ? t == query.tag : caseInsensitiveCompare(t, query.tag)) {
                tagMatch = true; break;
            }
        }
        if (!tagMatch) return false;
    }

    // State filter
    if (query.state != DSPState::Unknown) {
        if (pkg.state != query.state) return false;
    }

    // Origin filter
    if (query.origin != DSPOrigin::Unknown) {
        if (pkg.origin != query.origin) return false;
    }

    return true;
}

} // namespace dsp
} // namespace mbootcore
