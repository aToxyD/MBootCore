#pragma once

#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>
#include <sdk/PluginManifest.hpp>

#include <string>
#include <vector>

namespace mbootcore {
namespace sdk {

struct ValidationResult {
    bool valid{true};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class PluginValidator {
public:
    PluginValidator();

    ValidationResult validateVendor(const VendorRegistration& reg) const;
    ValidationResult validateProtocol(const ProtocolRegistration& reg) const;
    ValidationResult validateTransport(const TransportRegistration& reg) const;
    ValidationResult validateWorkflow(const WorkflowRegistration& reg) const;
    ValidationResult validateJob(const JobRegistration& reg) const;
    ValidationResult validatePackage(const PackageRegistration& reg) const;
    ValidationResult validateDiscovery(const DiscoveryRegistration& reg) const;
    ValidationResult validateCapability(const CapabilityRegistration& reg) const;
    ValidationResult validateManifest(const PluginManifest& manifest) const;

    ValidationResult validateAll(const PluginManifest& manifest,
                                 const std::vector<VendorRegistration>& vendors,
                                 const std::vector<ProtocolRegistration>& protocols,
                                 const std::vector<TransportRegistration>& transports,
                                 const std::vector<WorkflowRegistration>& workflows,
                                 const std::vector<JobRegistration>& jobs,
                                 const std::vector<PackageRegistration>& packages,
                                 const std::vector<DiscoveryRegistration>& discoveries,
                                 const std::vector<CapabilityRegistration>& capabilities) const;

    bool isValidName(const std::string& name) const;
    bool isValidVersion(const std::string& version) const;

private:
};

} // namespace sdk
} // namespace mbootcore
