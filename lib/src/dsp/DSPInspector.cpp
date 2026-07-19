#include <mbootcore/dsp/DSPInspector.hpp>
#include <mbootcore/dsp/DSPValidator.hpp>

#include "SafeParser.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <iomanip>
#include <set>
#include <map>
#include <utility>

namespace fs = std::filesystem;

namespace mbootcore {
namespace dsp {

namespace {



std::string extractValue(const std::string& line, const std::string& key) {
    auto pos = line.find('"' + key + '"');
    if (pos == std::string::npos) return {};
    auto colon = line.find(':', pos);
    if (colon == std::string::npos) return {};
    auto valStart = line.find_first_not_of(" \t\":", colon + 1);
    if (valStart == std::string::npos) return {};
    auto valEnd = line.find_first_of(",\n\r}", valStart);
    if (valEnd == std::string::npos) valEnd = line.size();
    auto val = line.substr(valStart, valEnd - valStart);
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    val.erase(std::remove(val.begin(), val.end(), ','), val.end());
    auto f = std::find_if_not(val.begin(), val.end(), [](unsigned char c){ return std::isspace(c); });
    val.erase(val.begin(), f);
    std::reverse(val.begin(), val.end());
    f = std::find_if_not(val.begin(), val.end(), [](unsigned char c){ return std::isspace(c); });
    val.erase(val.begin(), f);
    std::reverse(val.begin(), val.end());
    return val;
}

DSPVersion parseVersion(const std::string& ver) {
    DSPVersion v;
    auto p1 = ver.find('.');
    if (p1 != std::string::npos) {
        auto r1 = fromCharsUint32(ver.substr(0, p1));
        if (r1.ok) v.major = r1.value;
        auto p2 = ver.find('.', p1 + 1);
        if (p2 != std::string::npos) {
            auto r2 = fromCharsUint32(ver.substr(p1 + 1, p2 - p1 - 1));
            auto r3 = fromCharsUint32(ver.substr(p2 + 1));
            if (r2.ok) v.minor = r2.value;
            if (r3.ok) v.patch = r3.value;
        } else {
            auto r2 = fromCharsUint32(ver.substr(p1 + 1));
            if (r2.ok) v.minor = r2.value;
        }
    } else if (!ver.empty()) {
        auto r = fromCharsUint32(ver);
        if (r.ok) v.major = r.value;
    }
    return v;
}

std::vector<std::string> extractArray(const std::string& content, const std::string& key) {
    std::vector<std::string> result;
    auto pos = content.find('"' + key + '"');
    if (pos == std::string::npos) return result;
    auto colon = content.find(':', pos);
    if (colon == std::string::npos) return result;
    auto bracket = content.find('[', colon);
    if (bracket == std::string::npos) return result;
    auto close = content.find(']', bracket);
    if (close == std::string::npos) return result;
    auto arr = content.substr(bracket + 1, close - bracket - 1);
    std::string::size_type s = 0;
    while (true) {
        auto q1 = arr.find('"', s);
        if (q1 == std::string::npos) break;
        auto q2 = arr.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        result.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
        s = q2 + 1;
    }
    return result;
}

std::vector<std::string> collectFiles(const fs::path& dir) {
    std::vector<std::string> files;
    if (!fs::exists(dir)) return files;
    for (auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            files.push_back(fs::relative(entry.path(), dir).string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

uint64_t computeDirSize(const fs::path& dir) {
    uint64_t total = 0;
    if (!fs::exists(dir)) return 0;
    for (auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            total += entry.file_size();
        }
    }
    return total;
}

} // anonymous namespace

struct DSPInspector::Impl {
    std::string m_packagePath;
    DSPPackageMetadata m_metadata;
    bool m_open{false};

    Result<void> loadManifest();
    Result<void> loadVendor();
    Result<void> loadChipsets();
    Result<void> loadBootModes();
    Result<void> loadTransports();
    Result<void> loadProtocols();
    Result<void> loadQuirks();
    Result<void> loadProfiles();
};

Result<void> DSPInspector::Impl::loadManifest() {
    auto path = fs::path(m_packagePath) / "manifest.json";
    if (!fs::exists(path)) {
        return ErrorCode::Unknown;
    }
    std::ifstream mf(path);
    if (!mf) {
        return ErrorCode::Unknown;
    }
    std::stringstream ss;
    ss << mf.rdbuf();
    auto content = ss.str();

    m_metadata.manifest.formatVersion = extractValue(content, "formatVersion");
    m_metadata.manifest.packageId = extractValue(content, "packageId");
    m_metadata.manifest.name = extractValue(content, "name");
    m_metadata.manifest.vendor = extractValue(content, "vendor");
    m_metadata.manifest.description = extractValue(content, "description");
    auto verStr = extractValue(content, "version");
    if (!verStr.empty()) m_metadata.manifest.version = parseVersion(verStr);
    m_metadata.manifest.authors = extractArray(content, "authors");
    m_metadata.manifest.license = extractValue(content, "license");
    m_metadata.manifest.tags = extractArray(content, "tags");
    auto minCore = extractValue(content, "minCoreVersion");
    if (!minCore.empty()) m_metadata.manifest.minCoreVersion = minCore;
    auto maxCore = extractValue(content, "maxCoreVersion");
    if (!maxCore.empty()) m_metadata.manifest.maxCoreVersion = maxCore;
    auto compressStr = extractValue(content, "compressLoaders");
    m_metadata.manifest.compressLoaders = (compressStr == "true");
    auto csStr = extractValue(content, "checksum");
    if (!csStr.empty()) { auto r = fromCharsUint32(csStr); if (r.ok) m_metadata.manifest.checksum = r.value; }

    m_metadata.dspFormatVersion = parseVersion(m_metadata.manifest.formatVersion);
    return {};
}

Result<void> DSPInspector::Impl::loadVendor() {
    auto path = fs::path(m_packagePath) / "vendor.json";
    if (!fs::exists(path)) {
        return {};
    }
    std::ifstream mf(path);
    if (!mf) return ErrorCode::Unknown;
    std::stringstream ss;
    ss << mf.rdbuf();
    auto content = ss.str();

    auto vendorStr = extractValue(content, "vendor");
    if (vendorStr == "Qualcomm") m_metadata.vendor.vendorId = discovery::Vendor::Qualcomm;
    else if (vendorStr == "MediaTek") m_metadata.vendor.vendorId = discovery::Vendor::MediaTek;
    else if (vendorStr == "UNISOC") m_metadata.vendor.vendorId = discovery::Vendor::UNISOC;
    else if (vendorStr == "Samsung") m_metadata.vendor.vendorId = discovery::Vendor::Samsung;
    else if (vendorStr == "Rockchip") m_metadata.vendor.vendorId = discovery::Vendor::Rockchip;
    m_metadata.vendor.vendorName = vendorStr;
    m_metadata.vendor.displayName = extractValue(content, "displayName");
    m_metadata.vendor.website = extractValue(content, "website");
    m_metadata.vendor.aliases = extractArray(content, "aliases");
    m_metadata.vendor.supportedChipsets = extractArray(content, "supportedChipsets");
    m_metadata.vendor.supportedBootModes = extractArray(content, "supportedBootModes");
    m_metadata.vendor.supportedProtocols = extractArray(content, "supportedProtocols");
    m_metadata.vendor.supportedTransports = extractArray(content, "supportedTransports");
    return {};
}

Result<void> DSPInspector::Impl::loadChipsets() {
    auto path = fs::path(m_packagePath) / "chipsets.json";
    if (!fs::exists(path)) {
        return {};
    }
    std::ifstream mf(path);
    if (!mf) return ErrorCode::Unknown;
    std::stringstream ss;
    ss << mf.rdbuf();
    auto content = ss.str();

    auto chipsetsArray = extractArray(content, "chipsets");
    for (const auto& cs : chipsetsArray) {
        DSPChipsetMetadata cm;
        cm.id.vendor = cs;
        cm.displayName = cs;
        m_metadata.chipsets.push_back(std::move(cm));
    }

    auto chipObj = content.find("\"chipset\"");
    if (chipObj != std::string::npos) {
        DSPChipsetMetadata cm;
        cm.id.vendor = extractValue(content, "vendor");
        cm.id.family = extractValue(content, "family");
        cm.displayName = extractValue(content, "displayName");
        cm.manufacturer = extractValue(content, "manufacturer");
        cm.architecture = extractValue(content, "architecture");
        cm.variants = extractArray(content, "variants");
        cm.compatibleLoaders = extractArray(content, "compatibleLoaders");
        auto maxMem = extractValue(content, "maxMemoryMB");
        if (!maxMem.empty()) { auto r = fromCharsUint32(maxMem); if (r.ok) cm.maxMemoryMB = r.value; }
        auto maxFlash = extractValue(content, "maxFlashSizeMB");
        if (!maxFlash.empty()) { auto r = fromCharsUint32(maxFlash); if (r.ok) cm.maxFlashSizeMB = r.value; }
        auto tz = extractValue(content, "hasTrustZone");
        cm.hasTrustZone = (tz == "true");
        auto sb = extractValue(content, "hasSecureBoot");
        cm.hasSecureBoot = (sb == "true");
        m_metadata.chipsets.push_back(std::move(cm));
    }

    return {};
}

Result<void> DSPInspector::Impl::loadBootModes() {
    auto path = fs::path(m_packagePath) / "bootmodes.json";
    if (!fs::exists(path)) {
        return {};
    }
    std::ifstream mf(path);
    if (!mf) return ErrorCode::Unknown;
    std::stringstream ss;
    ss << mf.rdbuf();
    auto content = ss.str();

    auto modes = extractArray(content, "bootModes");
    for (const auto& mode : modes) {
        DSPBootModeMetadata bm;
        bm.bootModeId = mode;
        if (mode == "EDL") bm.bootMode = discovery::BootMode::EDL;
        else if (mode == "Firehose") bm.bootMode = discovery::BootMode::Firehose;
        else if (mode == "Fastboot") bm.bootMode = discovery::BootMode::Fastboot;
        else if (mode == "BootROM") bm.bootMode = discovery::BootMode::BootROM;
        else if (mode == "Preloader") bm.bootMode = discovery::BootMode::Preloader;
        bm.displayName = mode;
        m_metadata.bootModes.push_back(std::move(bm));
    }

    auto timeoutStr = extractValue(content, "defaultTimeoutMs");
    if (!timeoutStr.empty()) {
        auto r = fromCharsUint32(timeoutStr);
        if (r.ok) {
            for (auto& bm : m_metadata.bootModes) {
                bm.defaultTimeoutMs = r.value;
            }
        }
    }
    return {};
}

Result<void> DSPInspector::Impl::loadTransports() {
    auto path = fs::path(m_packagePath) / "transports.json";
    if (!fs::exists(path)) {
        return {};
    }
    std::ifstream mf(path);
    if (!mf) return ErrorCode::Unknown;
    std::stringstream ss;
    ss << mf.rdbuf();
    auto content = ss.str();

    auto transports = extractArray(content, "transports");
    for (const auto& t : transports) {
        DSPTransportMetadata tm;
        tm.transportId = t;
        if (t == "USB") tm.transportType = discovery::TransportType::USB;
        else if (t == "Serial") tm.transportType = discovery::TransportType::Serial;
        else if (t == "TCP") tm.transportType = discovery::TransportType::TCP;
        tm.displayName = t;
        m_metadata.transports.push_back(std::move(tm));
    }
    return {};
}

Result<void> DSPInspector::Impl::loadProtocols() {
    auto path = fs::path(m_packagePath) / "protocols.json";
    if (!fs::exists(path)) {
        return {};
    }
    std::ifstream mf(path);
    if (!mf) return ErrorCode::Unknown;
    std::stringstream ss;
    ss << mf.rdbuf();
    auto content = ss.str();

    auto protocols = extractArray(content, "protocols");
    for (const auto& p : protocols) {
        DSPProtocolMetadata pm;
        pm.protocolId = p;
        if (p == "Sahara") pm.protocolType = discovery::ProtocolType::Sahara;
        else if (p == "Firehose") pm.protocolType = discovery::ProtocolType::Firehose;
        else if (p == "Fastboot") pm.protocolType = discovery::ProtocolType::Fastboot;
        else if (p == "MediaTekBROM") pm.protocolType = discovery::ProtocolType::MediaTekBROM;
        else if (p == "UNISOCBootROM") pm.protocolType = discovery::ProtocolType::UNISOCBootROM;
        pm.displayName = p;
        m_metadata.protocols.push_back(std::move(pm));
    }
    return {};
}

Result<void> DSPInspector::Impl::loadQuirks() {
    auto quirksDir = fs::path(m_packagePath) / "quirks";
    if (fs::exists(quirksDir)) {
        for (auto& entry : fs::directory_iterator(quirksDir)) {
            if (entry.is_regular_file()) {
                m_metadata.quirks.push_back(entry.path().filename().string());
            }
        }
    }
    return {};
}

Result<void> DSPInspector::Impl::loadProfiles() {
    auto profilesDir = fs::path(m_packagePath) / "profiles";
    if (fs::exists(profilesDir)) {
        for (auto& entry : fs::directory_iterator(profilesDir)) {
            if (entry.is_regular_file()) {
                m_metadata.profiles.push_back(entry.path().filename().string());
            }
        }
    }
    return {};
}

DSPInspector::DSPInspector()
    : m_impl(std::make_unique<Impl>()) {
}

DSPInspector::~DSPInspector() = default;

Result<void> DSPInspector::open(const std::string& packagePath) {
    m_impl->m_packagePath = packagePath;
    m_impl->m_metadata = DSPPackageMetadata{};
    m_impl->m_open = false;

    if (!fs::exists(packagePath)) {
        return ErrorCode::Unknown;
    }

    auto result = m_impl->loadManifest();
    if (result.isError()) return result;

    result = m_impl->loadVendor();
    if (result.isError()) return result;

    result = m_impl->loadChipsets();
    if (result.isError()) return result;

    result = m_impl->loadBootModes();
    if (result.isError()) return result;

    result = m_impl->loadTransports();
    if (result.isError()) return result;

    result = m_impl->loadProtocols();
    if (result.isError()) return result;

    result = m_impl->loadQuirks();
    if (result.isError()) return result;

    result = m_impl->loadProfiles();
    if (result.isError()) return result;

    m_impl->m_open = true;
    return {};
}

DSPPackageMetadata DSPInspector::metadata() const {
    return m_impl->m_metadata;
}

std::vector<DSPChipsetMetadata> DSPInspector::chipsets() const {
    return m_impl->m_metadata.chipsets;
}

std::vector<DSPLoaderMetadata> DSPInspector::loaders() const {
    return m_impl->m_metadata.loaders;
}

std::vector<DSPBootModeMetadata> DSPInspector::bootModes() const {
    return m_impl->m_metadata.bootModes;
}

std::vector<DSPTransportMetadata> DSPInspector::transports() const {
    return m_impl->m_metadata.transports;
}

std::vector<DSPProtocolMetadata> DSPInspector::protocols() const {
    return m_impl->m_metadata.protocols;
}

std::vector<VendorQuirk> DSPInspector::quirks() const {
    std::vector<VendorQuirk> result;
    for (const auto& q : m_impl->m_metadata.quirks) {
        VendorQuirk vq;
        vq.quirkId = q;
        vq.name = q;
        result.push_back(vq);
    }
    return result;
}

std::vector<HardwareProfile> DSPInspector::profiles() const {
    std::vector<HardwareProfile> result;
    for (const auto& p : m_impl->m_metadata.profiles) {
        HardwareProfile hp;
        hp.profileId = p;
        hp.name = p;
        result.push_back(hp);
    }
    return result;
}

std::vector<std::string> DSPInspector::fileList() const {
    if (!m_impl->m_open) return {};
    return collectFiles(m_impl->m_packagePath);
}

std::vector<std::string> DSPInspector::supportedLocales() const {
    return m_impl->m_metadata.supportedLocales;
}

std::vector<std::string> DSPInspector::tags() const {
    return m_impl->m_metadata.manifest.tags;
}

DSPDependencyGraph DSPInspector::dependencyGraph() const {
    DSPDependencyGraph graph;
    if (m_impl->m_open) {
        (void)graph.addPackage(m_impl->m_metadata);
    }
    return graph;
}

DSPCompatibilityInfo DSPInspector::compatibilityInfo(const std::string& coreVersion) const {
    DSPCompatibilityInfo info;
    if (!m_impl->m_open) {
        info.errors.push_back("Package not open");
        return info;
    }

    auto pv = parseVersion(coreVersion);
    auto minV = parseVersion(m_impl->m_metadata.manifest.minCoreVersion);
    auto maxV = parseVersion(m_impl->m_metadata.manifest.maxCoreVersion);

    if (!m_impl->m_metadata.manifest.minCoreVersion.empty()) {
        info.coreCompatible = !(pv < minV);
        if (!info.coreCompatible) {
            info.errors.push_back("Core version " + coreVersion +
                " is below minimum " + m_impl->m_metadata.manifest.minCoreVersion);
        }
    } else {
        info.coreCompatible = true;
    }

    if (!m_impl->m_metadata.manifest.maxCoreVersion.empty()) {
        if (maxV < pv) {
            info.coreCompatible = false;
            info.errors.push_back("Core version " + coreVersion +
                " exceeds maximum " + m_impl->m_metadata.manifest.maxCoreVersion);
        }
    }

    info.sdkCompatible = true;
    info.osCompatible = true;
    info.architectureCompatible = true;
    return info;
}

DSPPackageStatistics DSPInspector::statistics() const {
    DSPPackageStatistics stats;
    if (!m_impl->m_open) return stats;

    stats.packageId = m_impl->m_metadata.manifest.packageId;
    stats.name = m_impl->m_metadata.manifest.name;
    stats.version = m_impl->m_metadata.manifest.version;
    stats.state = m_impl->m_metadata.state;
    stats.origin = m_impl->m_metadata.origin;
    stats.installedSize = computeDirSize(m_impl->m_packagePath);
    stats.loaderCount = static_cast<uint32_t>(m_impl->m_metadata.loaders.size());
    stats.chipsetCount = static_cast<uint32_t>(m_impl->m_metadata.chipsets.size());
    stats.profileCount = static_cast<uint32_t>(m_impl->m_metadata.profiles.size());
    stats.quirkCount = static_cast<uint32_t>(m_impl->m_metadata.quirks.size());
    stats.dependentPackages = static_cast<uint32_t>(m_impl->m_metadata.manifest.dependencies.size());
    stats.tags = m_impl->m_metadata.manifest.tags;
    stats.installDate = m_impl->m_metadata.installDate;
    return stats;
}

ValidationReport DSPInspector::healthReport() const {
    DSPValidator validator;
    if (!m_impl->m_open) {
        ValidationReport report;
        report.valid = false;
        report.errors.push_back({"", "Package not open", DSPError::PackageNotFound, true});
        return report;
    }
    return validator.validate(m_impl->m_packagePath, DSPValidationLevel::Full);
}

bool DSPInspector::isOpen() const noexcept {
    return m_impl->m_open;
}

void DSPInspector::close() {
    m_impl->m_open = false;
    m_impl->m_packagePath.clear();
    m_impl->m_metadata = DSPPackageMetadata{};
}

std::string DSPInspector::packagePath() const noexcept {
    return m_impl->m_packagePath;
}

} // namespace dsp
} // namespace mbootcore
