#include <sdk/VendorSDK.hpp>
#include <algorithm>
#include <iterator>

namespace mbootcore {
namespace sdk {

VendorSDK::VendorSDK()
    : m_manifest(std::make_unique<PluginManifest>())
    , m_validator(std::make_unique<PluginValidator>())
    , m_compatibility(std::make_unique<PluginCompatibility>())
    , m_depGraph(std::make_unique<PluginDependencyGraph>())
{
}

VendorSDK::~VendorSDK() = default;

VendorSDK& VendorSDK::registerVendor(const VendorRegistration& reg) {
    m_vendors.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerProtocol(const ProtocolRegistration& reg) {
    m_protocols.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerTransport(const TransportRegistration& reg) {
    m_transports.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerWorkflow(const WorkflowRegistration& reg) {
    m_workflows.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerJob(const JobRegistration& reg) {
    m_jobs.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerPackage(const PackageRegistration& reg) {
    m_packages.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerDiscovery(const DiscoveryRegistration& reg) {
    m_discoveries.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerCapability(const CapabilityRegistration& reg) {
    m_capabilities.push_back(reg);
    return *this;
}

VendorSDK& VendorSDK::registerVendors(const std::vector<VendorRegistration>& regs) {
    m_vendors.insert(m_vendors.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerProtocols(const std::vector<ProtocolRegistration>& regs) {
    m_protocols.insert(m_protocols.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerTransports(const std::vector<TransportRegistration>& regs) {
    m_transports.insert(m_transports.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerWorkflows(const std::vector<WorkflowRegistration>& regs) {
    m_workflows.insert(m_workflows.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerJobs(const std::vector<JobRegistration>& regs) {
    m_jobs.insert(m_jobs.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerPackages(const std::vector<PackageRegistration>& regs) {
    m_packages.insert(m_packages.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerDiscoveries(const std::vector<DiscoveryRegistration>& regs) {
    m_discoveries.insert(m_discoveries.end(), regs.begin(), regs.end());
    return *this;
}

VendorSDK& VendorSDK::registerCapabilities(const std::vector<CapabilityRegistration>& regs) {
    m_capabilities.insert(m_capabilities.end(), regs.begin(), regs.end());
    return *this;
}

SDKReport VendorSDK::finalize() {
    m_report = SDKReport{};
    m_report.valid = true;

    if (!validateRegistrations(m_report)) {
        m_report.valid = false;
    }

    if (!checkDuplicateRegistrations(m_report)) {
        m_report.valid = false;
    }

    if (!checkDependencies(m_report)) {
        m_report.valid = false;
    }

    if (!checkCompatibility(m_report)) {
        m_report.valid = false;
    }

    for (const auto& v : m_vendors) {
        m_report.registeredVendors.push_back(v.name);
        m_depGraph->addVendorNode(v);
    }
    for (const auto& p : m_protocols) {
        m_report.registeredProtocols.push_back(p.name);
        m_depGraph->addProtocolNode(p);
    }
    for (const auto& t : m_transports) {
        m_report.registeredTransports.push_back(t.name);
        m_depGraph->addTransportNode(t);
    }
    for (const auto& w : m_workflows) {
        m_report.registeredWorkflows.push_back(w.name);
        m_depGraph->addWorkflowNode(w);
    }
    for (const auto& j : m_jobs) {
        m_report.registeredJobs.push_back(j.name);
        m_depGraph->addJobNode(j);
    }
    for (const auto& p : m_packages) {
        m_report.registeredPackages.push_back(p.name);
        m_depGraph->addPackageNode(p);
    }
    for (const auto& d : m_discoveries) {
        m_report.registeredDiscoveries.push_back(d.name);
        m_depGraph->addDiscoveryNode(d);
    }
    for (const auto& c : m_capabilities) {
        m_report.registeredCapabilities.push_back(c.name);
        m_depGraph->addCapabilityNode(c);
    }

    auto depResult = m_depGraph->resolve();
    m_report.dependencyGraph = *m_depGraph;

    if (depResult.hasCircularDependencies) {
        m_report.valid = false;
        for (const auto& cycle : depResult.cycles) {
            std::string cycleStr;
            for (size_t i = 0; i < cycle.size(); ++i) {
                if (i > 0) cycleStr += " -> ";
                cycleStr += cycle[i];
            }
            m_report.errors.push_back("Circular dependency: " + cycleStr);
        }
    }

    m_finalized = true;
    return m_report;
}

const SDKReport& VendorSDK::report() const {
    return m_report;
}

void VendorSDK::clear() {
    m_vendors.clear();
    m_protocols.clear();
    m_transports.clear();
    m_workflows.clear();
    m_jobs.clear();
    m_packages.clear();
    m_discoveries.clear();
    m_capabilities.clear();
    m_report = SDKReport{};
    m_finalized = false;
}

bool VendorSDK::validateRegistrations(SDKReport& report) {
    bool valid = true;
    for (const auto& v : m_vendors) {
        auto vr = m_validator->validateVendor(v);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Vendor '" + v.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Vendor '" + v.name + "': " + w);
        }
    }
    for (const auto& p : m_protocols) {
        auto vr = m_validator->validateProtocol(p);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Protocol '" + p.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Protocol '" + p.name + "': " + w);
        }
    }
    for (const auto& t : m_transports) {
        auto vr = m_validator->validateTransport(t);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Transport '" + t.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Transport '" + t.name + "': " + w);
        }
    }
    for (const auto& w : m_workflows) {
        auto vr = m_validator->validateWorkflow(w);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Workflow '" + w.name + "': " + e);
            }
        }
        for (const auto& wmsg : vr.warnings) {
            report.warnings.push_back("Workflow '" + w.name + "': " + wmsg);
        }
    }
    for (const auto& j : m_jobs) {
        auto vr = m_validator->validateJob(j);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Job '" + j.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Job '" + j.name + "': " + w);
        }
    }
    for (const auto& p : m_packages) {
        auto vr = m_validator->validatePackage(p);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Package '" + p.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Package '" + p.name + "': " + w);
        }
    }
    for (const auto& d : m_discoveries) {
        auto vr = m_validator->validateDiscovery(d);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Discovery '" + d.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Discovery '" + d.name + "': " + w);
        }
    }
    for (const auto& c : m_capabilities) {
        auto vr = m_validator->validateCapability(c);
        if (!vr.valid) {
            valid = false;
            for (const auto& e : vr.errors) {
                report.errors.push_back("Capability '" + c.name + "': " + e);
            }
        }
        for (const auto& w : vr.warnings) {
            report.warnings.push_back("Capability '" + c.name + "': " + w);
        }
    }
    return valid;
}

bool VendorSDK::checkDuplicateRegistrations(SDKReport& report) {
    bool valid = true;
    auto checkDups = [&](const auto& vec, const std::string& type) {
        std::unordered_set<std::string> seen;
        for (const auto& item : vec) {
            if (!seen.insert(item.name).second) {
                valid = false;
                report.errors.push_back("Duplicate " + type + " registration: '" + item.name + "'");
            }
        }
    };
    checkDups(m_vendors, "vendor");
    checkDups(m_protocols, "protocol");
    checkDups(m_transports, "transport");
    checkDups(m_workflows, "workflow");
    checkDups(m_jobs, "job");
    checkDups(m_packages, "package");
    checkDups(m_discoveries, "discovery");
    checkDups(m_capabilities, "capability");
    return valid;
}

bool VendorSDK::checkDependencies(SDKReport& report) {
    (void)report;
    return true;
}

bool VendorSDK::checkCompatibility(SDKReport& report) {
    (void)report;
    return true;
}

} // namespace sdk
} // namespace mbootcore
