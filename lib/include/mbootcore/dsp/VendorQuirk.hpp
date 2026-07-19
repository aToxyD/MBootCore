#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>

namespace mbootcore {
namespace dsp {

// Quirk severity
enum class QuirkSeverity : uint32_t {
    Info     = 0,
    Warning  = 1,
    Error    = 2,
    Critical = 3
};

// Quirk scope
enum class QuirkScope : uint32_t {
    Global          = 0,
    Vendor          = 1,
    Chipset         = 2,
    Protocol        = 3,
    BootMode        = 4,
    Transport       = 5,
    Loader          = 6,
    Profile         = 7
};

// Quirk action
enum class QuirkAction : uint32_t {
    None          = 0,
    OverrideXML   = 1,
    OverrideReset = 2,
    OverrideTimeout = 3,
    OverrideACK   = 4,
    OverrideAlign = 5,
    OverrideGPTLayout = 6,
    OverrideAuth  = 7,
    OverrideConfig = 8,
    CustomAction   = 99
};

struct VendorQuirk {
    std::string quirkId;
    std::string name;
    std::string description;
    QuirkSeverity severity{QuirkSeverity::Info};
    QuirkScope scope{QuirkScope::Global};
    QuirkAction action{QuirkAction::None};
    discovery::Vendor vendor{discovery::Vendor::Unknown};
    ChipsetId chipset;
    discovery::ProtocolType protocol{discovery::ProtocolType::Unknown};
    discovery::BootMode bootMode{discovery::BootMode::Unknown};
    std::string target;
    std::string key;
    std::string value;
    std::unordered_map<std::string, std::string> parameters;
    std::vector<std::string> conditions;
    bool enabled{true};
    bool removable{true};
    DSPVersion addedInVersion;
    std::string documentation;
};

struct QuirkPolicy {
    std::string policyId;
    std::string name;
    std::string description;
    QuirkSeverity minSeverity{QuirkSeverity::Warning};
    bool autoApply{false};
    bool logOnly{false};
    bool blockOnCritical{true};
    std::vector<QuirkAction> allowedActions;
    std::vector<discovery::Vendor> applicableVendors;
};

class QuirkDatabase {
public:
    explicit QuirkDatabase(const DSPRepository& repo);
    ~QuirkDatabase();

    Result<void> rebuild();

    const VendorQuirk* findById(const std::string& quirkId) const;
    std::vector<const VendorQuirk*> findByVendor(discovery::Vendor vendor) const;
    std::vector<const VendorQuirk*> findByChipset(const ChipsetId& id) const;
    std::vector<const VendorQuirk*> findByProtocol(discovery::ProtocolType proto) const;
    std::vector<const VendorQuirk*> findByBootMode(discovery::BootMode mode) const;
    std::vector<const VendorQuirk*> findByScope(QuirkScope scope) const;
    std::vector<const VendorQuirk*> findBySeverity(QuirkSeverity severity) const;
    std::vector<const VendorQuirk*> findByAction(QuirkAction action) const;
    std::vector<const VendorQuirk*> search(const std::string& query) const;

    size_t count() const noexcept;
    bool isEmpty() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class QuirkResolver {
public:
    explicit QuirkResolver(const QuirkDatabase& db);

    struct ResolvedQuirks {
        std::vector<const VendorQuirk*> applicableQuirks;
        std::vector<const VendorQuirk*> infoMessages;
        std::vector<const VendorQuirk*> warnings;
        std::vector<const VendorQuirk*> errors;
        std::vector<const VendorQuirk*> criticals;
        bool hasBlockers{false};
    };

    ResolvedQuirks resolve(discovery::Vendor vendor, const ChipsetId& chipset,
        discovery::ProtocolType protocol = discovery::ProtocolType::Unknown,
        discovery::BootMode mode = discovery::BootMode::Unknown) const;

    std::vector<const VendorQuirk*> resolveForLoader(const std::string& loaderId) const;

    void setPolicy(const QuirkPolicy& policy);
    const QuirkPolicy& policy() const noexcept;

private:
    const QuirkDatabase& m_db;
    QuirkPolicy m_policy;
};

} // namespace dsp
} // namespace mbootcore
