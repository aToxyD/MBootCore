#include "mbootcore/dsp/VendorQuirk.hpp"
#include "mbootcore/dsp/DSPRepository.hpp"
#include "mbootcore/domain/Error.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace mbootcore {
namespace dsp {

namespace {

namespace fs = std::filesystem;

std::optional<nlohmann::json> parseQuirkFile(const std::string& path) {
    std::ifstream ifs{fs::path(path)};
    if (!ifs) return std::nullopt;

    try {
        nlohmann::json jv;
        ifs >> jv;
        if (!jv.is_object()) return std::nullopt;
        return jv;
    } catch (...) {
        return std::nullopt;
    }
}

discovery::Vendor parseVendor(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "QUALCOMM") return discovery::Vendor::Qualcomm;
    if (upper == "MEDIATEK") return discovery::Vendor::MediaTek;
    if (upper == "UNISOC") return discovery::Vendor::UNISOC;
    if (upper == "SAMSUNG") return discovery::Vendor::Samsung;
    if (upper == "ROCKCHIP") return discovery::Vendor::Rockchip;
    if (upper == "SPREADTRUM") return discovery::Vendor::Spreadtrum;
    if (upper == "APPLE") return discovery::Vendor::Apple;
    if (upper == "GOOGLE") return discovery::Vendor::Google;
    if (upper == "HUAWEI") return discovery::Vendor::Huawei;
    if (upper == "CUSTOM") return discovery::Vendor::Custom;
    return discovery::Vendor::Unknown;
}

discovery::ProtocolType parseProtocol(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "SAHARA") return discovery::ProtocolType::Sahara;
    if (upper == "FIREHOSE") return discovery::ProtocolType::Firehose;
    if (upper == "FASTBOOT") return discovery::ProtocolType::Fastboot;
    if (upper == "MEDIATEKBROM") return discovery::ProtocolType::MediaTekBROM;
    if (upper == "MEDIATEKDA") return discovery::ProtocolType::MediaTekDA;
    if (upper == "UNISOCBOOTROM") return discovery::ProtocolType::UNISOCBootROM;
    if (upper == "UNISOCFDL") return discovery::ProtocolType::UNISOCFDL;
    if (upper == "USBSTREAM") return discovery::ProtocolType::USBStream;
    if (upper == "CUSTOM") return discovery::ProtocolType::Custom;
    return discovery::ProtocolType::Unknown;
}

discovery::BootMode parseBootMode(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "BOOTROM") return discovery::BootMode::BootROM;
    if (upper == "EDL") return discovery::BootMode::EDL;
    if (upper == "FIREHOSE") return discovery::BootMode::Firehose;
    if (upper == "FASTBOOT") return discovery::BootMode::Fastboot;
    if (upper == "ADB") return discovery::BootMode::ADB;
    if (upper == "RECOVERY") return discovery::BootMode::Recovery;
    if (upper == "DOWNLOADMODE") return discovery::BootMode::DownloadMode;
    if (upper == "PRELOADER") return discovery::BootMode::Preloader;
    if (upper == "BROM") return discovery::BootMode::BROM;
    if (upper == "CUSTOM") return discovery::BootMode::Custom;
    return discovery::BootMode::Unknown;
}

QuirkSeverity parseQuirkSeverity(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "INFO") return QuirkSeverity::Info;
    if (upper == "WARNING") return QuirkSeverity::Warning;
    if (upper == "ERROR") return QuirkSeverity::Error;
    if (upper == "CRITICAL") return QuirkSeverity::Critical;
    return QuirkSeverity::Info;
}

QuirkScope parseQuirkScope(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "GLOBAL") return QuirkScope::Global;
    if (upper == "VENDOR") return QuirkScope::Vendor;
    if (upper == "CHIPSET") return QuirkScope::Chipset;
    if (upper == "PROTOCOL") return QuirkScope::Protocol;
    if (upper == "BOOTMODE") return QuirkScope::BootMode;
    if (upper == "TRANSPORT") return QuirkScope::Transport;
    if (upper == "LOADER") return QuirkScope::Loader;
    if (upper == "PROFILE") return QuirkScope::Profile;
    return QuirkScope::Global;
}

QuirkAction parseQuirkAction(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "NONE") return QuirkAction::None;
    if (upper == "OVERRIDEXML") return QuirkAction::OverrideXML;
    if (upper == "OVERRIDERESET") return QuirkAction::OverrideReset;
    if (upper == "OVERRIDETIMEOUT") return QuirkAction::OverrideTimeout;
    if (upper == "OVERRIDEACK") return QuirkAction::OverrideACK;
    if (upper == "OVERRIDEALIGN") return QuirkAction::OverrideAlign;
    if (upper == "OVERRIDEGPTLAYOUT") return QuirkAction::OverrideGPTLayout;
    if (upper == "OVERRIDEAUTH") return QuirkAction::OverrideAuth;
    if (upper == "OVERIDECONFIG") return QuirkAction::OverrideConfig;
    if (upper == "CUSTOMACTION") return QuirkAction::CustomAction;
    return QuirkAction::None;
}

bool matchVendor(discovery::Vendor a, discovery::Vendor b) {
    return a == b || a == discovery::Vendor::Unknown || b == discovery::Vendor::Unknown;
}

bool matchChipset(const ChipsetId& a, const ChipsetId& b) {
    if (a.vendor.empty() || b.vendor.empty()) return true;
    if (a.vendor != b.vendor) return false;
    if (!a.family.empty() && !b.family.empty() && a.family != b.family) return false;
    if (!a.variant.empty() && !b.variant.empty() && a.variant != b.variant) return false;
    return true;
}

} // anonymous namespace

// --- QuirkDatabase ---

struct QuirkDatabase::Impl {
    const DSPRepository& m_repo;
    std::unordered_map<std::string, VendorQuirk> m_quirks;
    std::unordered_multimap<discovery::Vendor, const VendorQuirk*> m_byVendor;
    std::unordered_multimap<std::string, const VendorQuirk*> m_byChipset;
    std::unordered_multimap<discovery::ProtocolType, const VendorQuirk*> m_byProtocol;
    std::unordered_multimap<discovery::BootMode, const VendorQuirk*> m_byBootMode;
    std::unordered_multimap<QuirkScope, const VendorQuirk*> m_byScope;
    std::unordered_multimap<QuirkSeverity, const VendorQuirk*> m_bySeverity;
    std::unordered_multimap<QuirkAction, const VendorQuirk*> m_byAction;
    bool m_built{false};

    explicit Impl(const DSPRepository& repo)
        : m_repo(repo) {}

    std::optional<VendorQuirk> loadQuirk(const std::string& path) {
        auto jsonOpt = parseQuirkFile(path);
        if (!jsonOpt) return std::nullopt;

        const auto& json = *jsonOpt;

        VendorQuirk quirk;
        quirk.quirkId = json.value("quirkId", std::string{});
        if (quirk.quirkId.empty()) {
            quirk.quirkId = fs::path(path).stem().string();
        }

        quirk.name = json.value("name", quirk.quirkId);
        quirk.description = json.value("description", std::string{});
        quirk.severity = parseQuirkSeverity(json.value("severity", std::string("INFO")));
        quirk.scope = parseQuirkScope(json.value("scope", std::string("GLOBAL")));
        quirk.action = parseQuirkAction(json.value("action", std::string("NONE")));
        quirk.vendor = parseVendor(json.value("vendor", std::string{}));

        quirk.chipset.vendor = json.value("chipsetVendor", std::string{});
        quirk.chipset.family = json.value("chipsetFamily", std::string{});
        quirk.chipset.variant = json.value("chipsetVariant", std::string{});

        quirk.protocol = parseProtocol(json.value("protocol", std::string{}));
        quirk.bootMode = parseBootMode(json.value("bootMode", std::string{}));

        quirk.target = json.value("target", std::string{});
        quirk.key = json.value("key", std::string{});
        quirk.value = json.value("value", std::string{});

        quirk.enabled = json.value("enabled", true);
        quirk.removable = json.value("removable", true);

        quirk.documentation = json.value("documentation", std::string{});

        quirk.addedInVersion.major = json.value("addedVersionMajor", 1u);
        quirk.addedInVersion.minor = json.value("addedVersionMinor", 0u);
        quirk.addedInVersion.patch = json.value("addedVersionPatch", 0u);

        for (const auto& [key, val] : json.items()) {
            if (key.rfind("param_", 0) == 0 && val.is_string()) {
                quirk.parameters[key.substr(6)] = val.get<std::string>();
            }
        }

        std::string conditions = json.value("conditions", std::string{});
        if (!conditions.empty()) {
            std::stringstream ss(conditions);
            std::string item;
            while (std::getline(ss, item, ',')) {
                if (!item.empty()) {
                    item.erase(0, item.find_first_not_of(' '));
                    item.erase(item.find_last_not_of(' ') + 1);
                    quirk.conditions.push_back(item);
                }
            }
        }

        return quirk;
    }

    Result<void> scanQuirks() {
        m_quirks.clear();
        m_byVendor.clear();
        m_byChipset.clear();
        m_byProtocol.clear();
        m_byBootMode.clear();
        m_byScope.clear();
        m_bySeverity.clear();
        m_byAction.clear();

        std::vector<std::string> searchPaths;
        std::string sysPath = m_repo.systemPath();
        std::string usrPath = m_repo.userPath();
        std::string portPath = m_repo.portablePath();

        if (!sysPath.empty()) searchPaths.push_back(sysPath + "/quirks");
        if (!usrPath.empty()) searchPaths.push_back(usrPath + "/quirks");
        if (!portPath.empty()) searchPaths.push_back(portPath + "/quirks");

        for (const auto& dirPath : searchPaths) {
            fs::path dir(dirPath);
            if (!fs::exists(dir) || !fs::is_directory(dir)) continue;

            for (const auto& entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().string();
                if (ext != ".json") continue;

                auto quirk = loadQuirk(entry.path().string());
                if (quirk) {
                    registerQuirk(std::move(*quirk));
                }
            }
        }

        m_built = true;
        return {};
    }

    void registerQuirk(VendorQuirk quirk) {
        const auto* ptr = &m_quirks.emplace(quirk.quirkId, std::move(quirk)).first->second;

        m_byVendor.emplace(ptr->vendor, ptr);

        std::string chipKey = ptr->chipset.toString();
        m_byChipset.emplace(chipKey, ptr);
        if (!ptr->chipset.vendor.empty()) {
            m_byChipset.emplace(ptr->chipset.vendor + "//", ptr);
        }

        m_byProtocol.emplace(ptr->protocol, ptr);
        m_byBootMode.emplace(ptr->bootMode, ptr);
        m_byScope.emplace(ptr->scope, ptr);
        m_bySeverity.emplace(ptr->severity, ptr);
        m_byAction.emplace(ptr->action, ptr);
    }

    std::vector<const VendorQuirk*> findQuirks(
        const std::vector<std::pair<discovery::Vendor, const ChipsetId*>>& filters) const {

        std::vector<const VendorQuirk*> result;
        std::unordered_set<std::string> seen;

        for (const auto& [vendor, chipset] : filters) {
            auto range = m_byVendor.equal_range(vendor);
            for (auto it = range.first; it != range.second; ++it) {
                if (seen.insert(it->second->quirkId).second) {
                    result.push_back(it->second);
                }
            }
        }

        return result;
    }
};

QuirkDatabase::QuirkDatabase(const DSPRepository& repo)
    : m_impl(std::make_unique<Impl>(repo)) {}

QuirkDatabase::~QuirkDatabase() = default;

Result<void> QuirkDatabase::rebuild() {
    return m_impl->scanQuirks();
}

const VendorQuirk* QuirkDatabase::findById(const std::string& quirkId) const {
    if (!m_impl->m_built) return nullptr;
    auto it = m_impl->m_quirks.find(quirkId);
    return it != m_impl->m_quirks.end() ? &it->second : nullptr;
}

std::vector<const VendorQuirk*> QuirkDatabase::findByVendor(discovery::Vendor vendor) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;
    auto range = m_impl->m_byVendor.equal_range(vendor);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::findByChipset(const ChipsetId& id) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;

    // Exact match first
    std::string chipKey = id.toString();
    auto range = m_impl->m_byChipset.equal_range(chipKey);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }

    // Partial matches
    for (const auto& [key, ptr] : m_impl->m_byChipset) {
        if (key == chipKey) continue;
        if (ptr->chipset.vendor == id.vendor &&
            (id.family.empty() || ptr->chipset.family == id.family ||
             ptr->chipset.family.empty()) &&
            (id.variant.empty() || ptr->chipset.variant == id.variant ||
             ptr->chipset.variant.empty())) {
            if (std::find(result.begin(), result.end(), ptr) == result.end()) {
                result.push_back(ptr);
            }
        }
    }

    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::findByProtocol(discovery::ProtocolType proto) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;
    auto range = m_impl->m_byProtocol.equal_range(proto);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::findByBootMode(discovery::BootMode mode) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;
    auto range = m_impl->m_byBootMode.equal_range(mode);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::findByScope(QuirkScope scope) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;
    auto range = m_impl->m_byScope.equal_range(scope);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::findBySeverity(QuirkSeverity severity) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;
    auto range = m_impl->m_bySeverity.equal_range(severity);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::findByAction(QuirkAction action) const {
    if (!m_impl->m_built) return {};
    std::vector<const VendorQuirk*> result;
    auto range = m_impl->m_byAction.equal_range(action);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<const VendorQuirk*> QuirkDatabase::search(const std::string& query) const {
    if (!m_impl->m_built || query.empty()) return {};

    std::vector<const VendorQuirk*> result;
    std::string lower = query;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto& [id, quirk] : m_impl->m_quirks) {
        if (!quirk.enabled) continue;

        std::string nameLower = quirk.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        std::string descLower = quirk.description;
        std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);
        std::string idLower = quirk.quirkId;
        std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);

        if (nameLower.find(lower) != std::string::npos ||
            descLower.find(lower) != std::string::npos ||
            idLower.find(lower) != std::string::npos) {
            result.push_back(&quirk);
        }
    }

    return result;
}

size_t QuirkDatabase::count() const noexcept {
    if (!m_impl->m_built) return 0;
    return m_impl->m_quirks.size();
}

bool QuirkDatabase::isEmpty() const noexcept {
    if (!m_impl->m_built) return true;
    return m_impl->m_quirks.empty();
}

// --- QuirkResolver ---

QuirkResolver::QuirkResolver(const QuirkDatabase& db)
    : m_db(db)
    , m_policy() {}

QuirkResolver::ResolvedQuirks QuirkResolver::resolve(
    discovery::Vendor vendor, const ChipsetId& chipset,
    discovery::ProtocolType protocol, discovery::BootMode mode) const {

    ResolvedQuirks result;

    auto allQuirks = m_db.findByVendor(vendor);

    auto chipsetQuirks = m_db.findByChipset(chipset);
    allQuirks.insert(allQuirks.end(), chipsetQuirks.begin(), chipsetQuirks.end());

    auto protoQuirks = m_db.findByProtocol(protocol);
    allQuirks.insert(allQuirks.end(), protoQuirks.begin(), protoQuirks.end());

    auto modeQuirks = m_db.findByBootMode(mode);
    allQuirks.insert(allQuirks.end(), modeQuirks.begin(), modeQuirks.end());

    // Global quirks
    auto globalQuirks = m_db.findByScope(QuirkScope::Global);
    allQuirks.insert(allQuirks.end(), globalQuirks.begin(), globalQuirks.end());

    // Deduplicate
    std::unordered_set<std::string> seen;
    std::vector<const VendorQuirk*> unique;
    for (const auto* q : allQuirks) {
        if (!q->enabled) continue;
        if (!m_policy.applicableVendors.empty()) {
            bool allowed = false;
            for (auto av : m_policy.applicableVendors) {
                if (av == q->vendor || q->vendor == discovery::Vendor::Unknown) {
                    allowed = true;
                    break;
                }
            }
            if (!allowed) continue;
        }

        if (seen.insert(q->quirkId).second) {
            unique.push_back(q);
        }
    }

    // Filter by matched criteria
    for (const auto* q : unique) {
        bool matches = true;

        if (q->vendor != discovery::Vendor::Unknown && !matchVendor(q->vendor, vendor)) {
            matches = false;
        }
        if (matches && !q->chipset.vendor.empty() && !matchChipset(q->chipset, chipset)) {
            matches = false;
        }
        if (matches && q->protocol != discovery::ProtocolType::Unknown &&
            q->protocol != protocol) {
            matches = false;
        }
        if (matches && q->bootMode != discovery::BootMode::Unknown &&
            q->bootMode != mode) {
            matches = false;
        }
        if (matches && q->severity < m_policy.minSeverity) {
            matches = false;
        }

        if (std::find(result.applicableQuirks.begin(), result.applicableQuirks.end(), q) !=
            result.applicableQuirks.end()) {
            // Skip if already added
            continue;
        }

        if (matches) {
            result.applicableQuirks.push_back(q);

            switch (q->severity) {
                case QuirkSeverity::Info:
                    result.infoMessages.push_back(q);
                    break;
                case QuirkSeverity::Warning:
                    result.warnings.push_back(q);
                    break;
                case QuirkSeverity::Error:
                    result.errors.push_back(q);
                    break;
                case QuirkSeverity::Critical:
                    result.criticals.push_back(q);
                    result.hasBlockers = true;
                    break;
            }
        }
    }

    return result;
}

std::vector<const VendorQuirk*> QuirkResolver::resolveForLoader(const std::string& loaderId) const {
    std::vector<const VendorQuirk*> result;

    auto scopeQuirks = m_db.findByScope(QuirkScope::Loader);
    for (const auto* q : scopeQuirks) {
        if (!q->enabled) continue;
        if (q->target.empty() || q->target == loaderId) {
            result.push_back(q);
        }
    }

    return result;
}

void QuirkResolver::setPolicy(const QuirkPolicy& policy) {
    m_policy = policy;
}

const QuirkPolicy& QuirkResolver::policy() const noexcept {
    return m_policy;
}

} // namespace dsp
} // namespace mbootcore
