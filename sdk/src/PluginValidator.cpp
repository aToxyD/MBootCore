#include <sdk/PluginValidator.hpp>
#include <algorithm>
#include <cctype>

namespace mbootcore {
namespace sdk {

PluginValidator::PluginValidator() = default;

bool PluginValidator::isValidName(const std::string& name) const {
    if (name.empty()) return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.';
    });
}

bool PluginValidator::isValidVersion(const std::string& version) const {
    if (version.empty()) return false;
    int dots = 0;
    for (char c : version) {
        if (c == '.') { ++dots; continue; }
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return dots >= 0 && dots <= 3;
}

ValidationResult PluginValidator::validateVendor(const VendorRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid vendor name: '" + reg.name + "'");
    }
    if (reg.name.empty()) {
        r.valid = false;
        r.errors.push_back("Vendor name is required");
    }
    if (reg.usbVids.empty() && reg.vendorId == mbootcore::discovery::Vendor::Custom) {
        r.warnings.push_back("No USB VID specified for custom vendor");
    }
    return r;
}

ValidationResult PluginValidator::validateProtocol(const ProtocolRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid protocol name: '" + reg.name + "'");
    }
    if (reg.protocolType == mbootcore::discovery::ProtocolType::Unknown) {
        r.valid = false;
        r.errors.push_back("Protocol type must be specified");
    }
    if (reg.supportedTransports.empty()) {
        r.warnings.push_back("No supported transports specified");
    }
    return r;
}

ValidationResult PluginValidator::validateTransport(const TransportRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid transport name: '" + reg.name + "'");
    }
    if (reg.transportType == mbootcore::discovery::TransportType::Unknown) {
        r.warnings.push_back("Transport type is Unknown");
    }
    return r;
}

ValidationResult PluginValidator::validateWorkflow(const WorkflowRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid workflow name: '" + reg.name + "'");
    }
    return r;
}

ValidationResult PluginValidator::validateJob(const JobRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid job name: '" + reg.name + "'");
    }
    if (reg.jobType.empty()) {
        r.warnings.push_back("Job type is empty");
    }
    return r;
}

ValidationResult PluginValidator::validatePackage(const PackageRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid package name: '" + reg.name + "'");
    }
    return r;
}

ValidationResult PluginValidator::validateDiscovery(const DiscoveryRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid discovery name: '" + reg.name + "'");
    }
    return r;
}

ValidationResult PluginValidator::validateCapability(const CapabilityRegistration& reg) const {
    ValidationResult r;
    if (!isValidName(reg.name)) {
        r.valid = false;
        r.errors.push_back("Invalid capability name: '" + reg.name + "'");
    }
    return r;
}

ValidationResult PluginValidator::validateManifest(const PluginManifest& manifest) const {
    ValidationResult r;
    auto errors = manifest.validate();
    if (!errors.empty()) {
        r.valid = false;
        for (const auto& e : errors) {
            r.errors.push_back(e);
        }
    }
    return r;
}

ValidationResult PluginValidator::validateAll(
    const PluginManifest& manifest,
    const std::vector<VendorRegistration>& vendors,
    const std::vector<ProtocolRegistration>& protocols,
    const std::vector<TransportRegistration>& transports,
    const std::vector<WorkflowRegistration>& workflows,
    const std::vector<JobRegistration>& jobs,
    const std::vector<PackageRegistration>& packages,
    const std::vector<DiscoveryRegistration>& discoveries,
    const std::vector<CapabilityRegistration>& capabilities) const
{
    ValidationResult combined;
    auto mr = validateManifest(manifest);
    if (!mr.valid) {
        combined.valid = false;
        combined.errors.insert(combined.errors.end(), mr.errors.begin(), mr.errors.end());
    }
    auto check = [&](const auto& vr) {
        if (!vr.valid) {
            combined.valid = false;
            combined.errors.insert(combined.errors.end(), vr.errors.begin(), vr.errors.end());
        }
        combined.warnings.insert(combined.warnings.end(), vr.warnings.begin(), vr.warnings.end());
    };
    for (const auto& v : vendors) check(validateVendor(v));
    for (const auto& p : protocols) check(validateProtocol(p));
    for (const auto& t : transports) check(validateTransport(t));
    for (const auto& w : workflows) check(validateWorkflow(w));
    for (const auto& j : jobs) check(validateJob(j));
    for (const auto& p : packages) check(validatePackage(p));
    for (const auto& d : discoveries) check(validateDiscovery(d));
    for (const auto& c : capabilities) check(validateCapability(c));
    return combined;
}

} // namespace sdk
} // namespace mbootcore
