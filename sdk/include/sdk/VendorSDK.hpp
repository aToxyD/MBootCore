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
#include <sdk/PluginValidator.hpp>
#include <sdk/PluginCompatibility.hpp>
#include <sdk/PluginDependencyGraph.hpp>

#include <mbootcore/domain/Error.hpp>

#include <memory>
#include <string>
#include <vector>

namespace mbootcore {
namespace sdk {

struct SDKReport {
    bool valid{false};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> registeredVendors;
    std::vector<std::string> registeredProtocols;
    std::vector<std::string> registeredTransports;
    std::vector<std::string> registeredWorkflows;
    std::vector<std::string> registeredJobs;
    std::vector<std::string> registeredPackages;
    std::vector<std::string> registeredDiscoveries;
    std::vector<std::string> registeredCapabilities;
    PluginDependencyGraph dependencyGraph;
};

class VendorSDK {
public:
    VendorSDK();
    ~VendorSDK();

    VendorSDK(const VendorSDK&) = delete;
    VendorSDK& operator=(const VendorSDK&) = delete;
    VendorSDK(VendorSDK&&) = delete;
    VendorSDK& operator=(VendorSDK&&) = delete;

    // Registration methods
    VendorSDK& registerVendor(const VendorRegistration& reg);
    VendorSDK& registerProtocol(const ProtocolRegistration& reg);
    VendorSDK& registerTransport(const TransportRegistration& reg);
    VendorSDK& registerWorkflow(const WorkflowRegistration& reg);
    VendorSDK& registerJob(const JobRegistration& reg);
    VendorSDK& registerPackage(const PackageRegistration& reg);
    VendorSDK& registerDiscovery(const DiscoveryRegistration& reg);
    VendorSDK& registerCapability(const CapabilityRegistration& reg);

    // Batch registration
    VendorSDK& registerVendors(const std::vector<VendorRegistration>& regs);
    VendorSDK& registerProtocols(const std::vector<ProtocolRegistration>& regs);
    VendorSDK& registerTransports(const std::vector<TransportRegistration>& regs);
    VendorSDK& registerWorkflows(const std::vector<WorkflowRegistration>& regs);
    VendorSDK& registerJobs(const std::vector<JobRegistration>& regs);
    VendorSDK& registerPackages(const std::vector<PackageRegistration>& regs);
    VendorSDK& registerDiscoveries(const std::vector<DiscoveryRegistration>& regs);
    VendorSDK& registerCapabilities(const std::vector<CapabilityRegistration>& regs);

    // Finalization
    SDKReport finalize();

    // Query after finalization
    bool isFinalized() const noexcept { return m_finalized; }
    const SDKReport& report() const;

    // Accessors
    const std::vector<VendorRegistration>& vendors() const noexcept { return m_vendors; }
    const std::vector<ProtocolRegistration>& protocols() const noexcept { return m_protocols; }
    const std::vector<TransportRegistration>& transports() const noexcept { return m_transports; }
    const std::vector<WorkflowRegistration>& workflows() const noexcept { return m_workflows; }
    const std::vector<JobRegistration>& jobs() const noexcept { return m_jobs; }
    const std::vector<PackageRegistration>& packages() const noexcept { return m_packages; }
    const std::vector<DiscoveryRegistration>& discoveries() const noexcept { return m_discoveries; }
    const std::vector<CapabilityRegistration>& capabilities() const noexcept { return m_capabilities; }

    // Clear all registrations
    void clear();

private:
    bool validateRegistrations(SDKReport& report);
    bool checkDuplicateRegistrations(SDKReport& report);
    bool checkDependencies(SDKReport& report);
    bool checkCompatibility(SDKReport& report);

    std::vector<VendorRegistration> m_vendors;
    std::vector<ProtocolRegistration> m_protocols;
    std::vector<TransportRegistration> m_transports;
    std::vector<WorkflowRegistration> m_workflows;
    std::vector<JobRegistration> m_jobs;
    std::vector<PackageRegistration> m_packages;
    std::vector<DiscoveryRegistration> m_discoveries;
    std::vector<CapabilityRegistration> m_capabilities;

    std::unique_ptr<PluginManifest> m_manifest;
    std::unique_ptr<PluginValidator> m_validator;
    std::unique_ptr<PluginCompatibility> m_compatibility;
    std::unique_ptr<PluginDependencyGraph> m_depGraph;

    bool m_finalized{false};
    SDKReport m_report;
};

} // namespace sdk
} // namespace mbootcore
