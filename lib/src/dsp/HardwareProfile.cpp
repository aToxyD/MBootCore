#include "mbootcore/dsp/HardwareProfile.hpp"
#include "mbootcore/dsp/DSPRepository.hpp"
#include "mbootcore/domain/Error.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

namespace mbootcore {
namespace dsp {

namespace {

namespace fs = std::filesystem;

ChipsetId chipsetFromStrings(const std::string& vendor,
                              const std::string& family,
                              const std::string& variant) {
    ChipsetId id;
    id.vendor = vendor;
    id.family = family;
    id.variant = variant;
    return id;
}

void parseStorageProfile(const nlohmann::json& jv, StorageProfile& storage) {
    std::string stype = jv.value("storageType", std::string("eMMC"));
    if (stype == "UFS") storage.type = StorageType::UFS;
    else if (stype == "NAND") storage.type = StorageType::NAND;
    else if (stype == "NOR") storage.type = StorageType::NOR;
    else if (stype == "SPI") storage.type = StorageType::SPI;
    else if (stype == "SD") storage.type = StorageType::SD;
    else if (stype == "NVMe") storage.type = StorageType::NVMe;
    else storage.type = StorageType::eMMC;

    storage.totalSizeMB = jv.value("totalSizeMB", static_cast<uint64_t>(0));
    storage.sectorSize = jv.value("sectorSize", 512u);
    storage.blockSize = jv.value("blockSize", 4096u);
    storage.eraseBlockSize = jv.value("eraseBlockSize", 4194304u);
    storage.maxPartitionSize = jv.value("maxPartitionSize", static_cast<uint64_t>(0));
    storage.maxPartitionCount = jv.value("maxPartitionCount", 32u);
    storage.supportsTRIM = jv.value("supportsTRIM", false);
    storage.supportsSanitize = jv.value("supportsSanitize", false);
    storage.supportsHardReset = jv.value("supportsHardReset", false);
    storage.supportsCache = jv.value("supportsCache", false);
    storage.maxWriteSpeedMBps = jv.value("maxWriteSpeedMBps", 0u);
    storage.maxReadSpeedMBps = jv.value("maxReadSpeedMBps", 0u);
}

void parseMemoryProfile(const nlohmann::json& jv, MemoryProfile& memory) {
    memory.totalMB = jv.value("totalMB", static_cast<uint64_t>(0));
    memory.availableMB = jv.value("availableMB", static_cast<uint64_t>(0));
    memory.reservedMB = jv.value("reservedMB", static_cast<uint64_t>(0));
    memory.ddrFreqMHz = jv.value("ddrFreqMHz", static_cast<uint64_t>(0));
    memory.channelCount = jv.value("channelCount", 1u);
    memory.eccSupported = jv.value("eccSupported", false);
    memory.sharedWithGPU = jv.value("sharedWithGPU", false);
}

void parseCapabilityProfile(const nlohmann::json& jv, CapabilityProfile& caps) {
    caps.canProgram = jv.value("canProgram", false);
    caps.canErase = jv.value("canErase", false);
    caps.canRead = jv.value("canRead", false);
    caps.canVerify = jv.value("canVerify", false);
    caps.canBackup = jv.value("canBackup", false);
    caps.canRestore = jv.value("canRestore", false);
    caps.canCompare = jv.value("canCompare", false);
    caps.canResume = jv.value("canResume", false);
    caps.canRollback = jv.value("canRollback", false);
    caps.canHotplug = jv.value("canHotplug", false);
    caps.canAsync = jv.value("canAsync", false);
    caps.canMultiSession = jv.value("canMultiSession", false);
    caps.canSparse = jv.value("canSparse", false);
    caps.canCompress = jv.value("canCompress", false);
    caps.canEncrypt = jv.value("canEncrypt", false);
    caps.canSign = jv.value("canSign", false);
    caps.canAuth = jv.value("canAuth", false);
}

void parseSecurityProfile(const nlohmann::json& jv, SecurityProfile& sec) {
    sec.secureBoot = jv.value("secureBoot", false);
    sec.trustZone = jv.value("trustZone", false);
    sec.hardwareCrypto = jv.value("hardwareCrypto", false);
    sec.debugPort = jv.value("debugPort", false);
    sec.signedImages = jv.value("signedImages", false);
    sec.encryptedStorage = jv.value("encryptedStorage", false);
    sec.authRequired = jv.value("authRequired", false);
    sec.rpmb = jv.value("rpmb", false);
    sec.maxAuthRetries = jv.value("maxAuthRetries", 3u);
}

void parsePerformanceProfile(const nlohmann::json& jv, PerformanceProfile& perf) {
    perf.recommendedWorkers = jv.value("recommendedWorkers", 4u);
    perf.maxWorkers = jv.value("maxWorkers", 8u);
    perf.bufferSizeKB = jv.value("bufferSizeKB", 1024u);
    perf.maxTransferSizeMB = jv.value("maxTransferSizeMB", 16u);
    perf.recommendedTimeoutMs = jv.value("recommendedTimeoutMs", 30000u);
    perf.useAsyncIO = jv.value("useAsyncIO", true);
    perf.useBulkTransfer = jv.value("useBulkTransfer", true);
    perf.useStreamMode = jv.value("useStreamMode", true);
    perf.maxRetries = jv.value("maxRetries", 3u);
    perf.retryDelay = std::chrono::milliseconds(
        static_cast<int64_t>(jv.value("retryDelay", 1000.0)));
}

void parseFlashGeometry(const nlohmann::json& jv, FlashGeometry& geom) {
    std::string stype = jv.value("storageType", std::string("eMMC"));
    if (stype == "UFS") geom.storageType = StorageType::UFS;
    else if (stype == "NAND") geom.storageType = StorageType::NAND;
    else if (stype == "NOR") geom.storageType = StorageType::NOR;
    else if (stype == "SPI") geom.storageType = StorageType::SPI;
    else if (stype == "SD") geom.storageType = StorageType::SD;
    else if (stype == "NVMe") geom.storageType = StorageType::NVMe;
    else geom.storageType = StorageType::eMMC;

    geom.totalSectors = jv.value("totalSectors", static_cast<uint64_t>(0));
    geom.sectorSize = jv.value("sectorSize", 512u);
    geom.totalSizeBytes = jv.value("totalSizeBytes", static_cast<uint64_t>(0));
    geom.logicalBlockSize = jv.value("logicalBlockSize", 512u);
    geom.physicalBlockSize = jv.value("physicalBlockSize", 4096u);
    geom.eraseGroupSize = jv.value("eraseGroupSize", 4194304u);
    geom.availableSectors = jv.value("availableSectors", static_cast<uint64_t>(0));
    geom.maxProgramSize = jv.value("maxProgramSize", static_cast<uint64_t>(0));
    geom.maxProgramChunks = jv.value("maxProgramChunks", 32u);
    geom.supportsSparse = jv.value("supportsSparse", false);
    geom.writeProtectSupported = jv.value("writeProtectSupported", false);
}

} // anonymous namespace

struct HardwareProfileManager::Impl {
    const DSPRepository& m_repo;
    std::unordered_map<std::string, HardwareProfile> m_profiles;
    std::unordered_map<std::string, std::vector<const HardwareProfile*>> m_byChipset;
    bool m_built{false};

    explicit Impl(const DSPRepository& repo)
        : m_repo(repo) {}

    std::optional<HardwareProfile> loadProfile(const std::string& path) {
        std::ifstream ifs{fs::path(path)};
        if (!ifs) return std::nullopt;

        nlohmann::json jv;
        try {
            ifs >> jv;
        } catch (...) {
            return std::nullopt;
        }

        if (!jv.is_object()) return std::nullopt;

        HardwareProfile profile;
        profile.profileId = jv.value("profileId", std::string{});
        if (profile.profileId.empty()) {
            profile.profileId = fs::path(path).stem().string();
        }

        profile.name = jv.value("name", profile.profileId);
        profile.description = jv.value("description", std::string{});
        profile.chipset = chipsetFromStrings(
            jv.value("chipsetVendor", std::string{}),
            jv.value("chipsetFamily", std::string{}),
            jv.value("chipsetVariant", std::string{}));
        profile.isDefault = jv.value("isDefault", false);

        parseStorageProfile(jv, profile.storage);
        parseMemoryProfile(jv, profile.memory);
        parseCapabilityProfile(jv, profile.capabilities);
        parseSecurityProfile(jv, profile.security);
        parsePerformanceProfile(jv, profile.performance);
        parseFlashGeometry(jv, profile.flashGeometry);

        for (const auto& [key, val] : jv.items()) {
            if (key.rfind("custom_", 0) == 0 && val.is_string()) {
                profile.customProperties[key.substr(7)] = val.get<std::string>();
            }
        }

        profile.version.major = jv.value("versionMajor", 1u);
        profile.version.minor = jv.value("versionMinor", 0u);
        profile.version.patch = jv.value("versionPatch", 0u);

        return profile;
    }

    Result<void> scanProfiles() {
        m_profiles.clear();
        m_byChipset.clear();

        std::vector<std::string> searchPaths;
        std::string sysPath = m_repo.systemPath();
        std::string usrPath = m_repo.userPath();
        std::string portPath = m_repo.portablePath();

        if (!sysPath.empty()) searchPaths.push_back(sysPath + "/profiles");
        if (!usrPath.empty()) searchPaths.push_back(usrPath + "/profiles");
        if (!portPath.empty()) searchPaths.push_back(portPath + "/profiles");

        for (const auto& dirPath : searchPaths) {
            fs::path dir(dirPath);
            if (!fs::exists(dir) || !fs::is_directory(dir)) continue;

            for (const auto& entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().string();
                if (ext != ".jv") continue;

                auto profile = loadProfile(entry.path().string());
                if (profile) {
                    m_profiles[profile->profileId] = std::move(*profile);
                }
            }
        }

        for (const auto& [id, profile] : m_profiles) {
            std::string chipKey = profile.chipset.toString();
            m_byChipset[chipKey].push_back(&m_profiles.at(id));
        }

        m_built = true;
        return {};
    }
};

HardwareProfileManager::HardwareProfileManager(const DSPRepository& repo)
    : m_impl(std::make_unique<Impl>(repo)) {}

HardwareProfileManager::~HardwareProfileManager() = default;

Result<void> HardwareProfileManager::rebuild() {
    return m_impl->scanProfiles();
}

const HardwareProfile* HardwareProfileManager::findById(const std::string& profileId) const {
    if (!m_impl->m_built) return nullptr;
    auto it = m_impl->m_profiles.find(profileId);
    if (it == m_impl->m_profiles.end()) return nullptr;
    return &it->second;
}

std::vector<const HardwareProfile*> HardwareProfileManager::findByChipset(const ChipsetId& id) const {
    if (!m_impl->m_built) return {};
    std::vector<const HardwareProfile*> result;
    std::string chipKey = id.toString();

    auto it = m_impl->m_byChipset.find(chipKey);
    if (it != m_impl->m_byChipset.end()) {
        result = it->second;
    }

    // Also search with partial chipset matching
    for (const auto& [pid, profile] : m_impl->m_profiles) {
        if (profile.chipset.vendor == id.vendor &&
            (id.family.empty() || profile.chipset.family == id.family) &&
            (id.variant.empty() || profile.chipset.variant == id.variant)) {
            if (std::find(result.begin(), result.end(), &profile) == result.end()) {
                result.push_back(&profile);
            }
        }
    }

    return result;
}

const HardwareProfile* HardwareProfileManager::findDefault(const ChipsetId& id) const {
    if (!m_impl->m_built) return nullptr;

    auto profiles = findByChipset(id);
    for (const auto* profile : profiles) {
        if (profile->isDefault) return profile;
    }

    if (!profiles.empty()) return profiles.front();
    return nullptr;
}

std::vector<const HardwareProfile*> HardwareProfileManager::search(const std::string& query) const {
    if (!m_impl->m_built || query.empty()) return {};

    std::vector<const HardwareProfile*> result;
    std::string lower = query;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto& [id, profile] : m_impl->m_profiles) {
        std::string nameLower = profile.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        std::string descLower = profile.description;
        std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);
        std::string idLower = profile.profileId;
        std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);
        std::string chipStr = profile.chipset.toString();
        std::transform(chipStr.begin(), chipStr.end(), chipStr.begin(), ::tolower);

        if (nameLower.find(lower) != std::string::npos ||
            descLower.find(lower) != std::string::npos ||
            idLower.find(lower) != std::string::npos ||
            chipStr.find(lower) != std::string::npos) {
            result.push_back(&profile);
        }
    }

    return result;
}

size_t HardwareProfileManager::count() const noexcept {
    if (!m_impl->m_built) return 0;
    return m_impl->m_profiles.size();
}

} // namespace dsp
} // namespace mbootcore
