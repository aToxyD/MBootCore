#pragma once

#include <sdk/PluginManifest.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/TransportRegistration.hpp>
#include <sdk/WorkflowRegistration.hpp>
#include <sdk/JobRegistration.hpp>
#include <sdk/PackageRegistration.hpp>
#include <sdk/DiscoveryRegistration.hpp>
#include <sdk/CapabilityRegistration.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace mbootcore {
namespace sdk {

struct GraphNode {
    std::string id;
    std::string type;
    std::string name;
    std::string version;
    std::vector<std::string> dependsOn;
    std::vector<std::string> dependedBy;
    bool isResolved{false};
};

struct DependencyGraphReport {
    bool hasCircularDependencies{false};
    std::vector<std::vector<std::string>> cycles;
    std::vector<std::string> unresolvedDependencies;
    std::vector<std::string> resolvedOrder;
    std::unordered_map<std::string, GraphNode> nodes;
};

class PluginDependencyGraph {
public:
    PluginDependencyGraph();

    void addNode(const std::string& id, const std::string& type,
                 const std::string& name, const std::string& version,
                 const std::vector<std::string>& dependencies);

    void addVendorNode(const VendorRegistration& reg);
    void addProtocolNode(const ProtocolRegistration& reg);
    void addTransportNode(const TransportRegistration& reg);
    void addWorkflowNode(const WorkflowRegistration& reg);
    void addJobNode(const JobRegistration& reg);
    void addPackageNode(const PackageRegistration& reg);
    void addDiscoveryNode(const DiscoveryRegistration& reg);
    void addCapabilityNode(const CapabilityRegistration& reg);

    DependencyGraphReport resolve();

    bool hasCycle() const noexcept { return m_hasCycle; }
    const std::vector<std::vector<std::string>>& cycles() const noexcept { return m_cycles; }

    std::string toDot() const;
    std::string toJson() const;
    std::string toPlantUml() const;

    void clear();

private:
    bool dfs(const std::string& nodeId,
             std::unordered_set<std::string>& visited,
             std::unordered_set<std::string>& inStack,
             std::vector<std::string>& stack);

    std::unordered_map<std::string, GraphNode> m_nodes;
    bool m_hasCycle{false};
    std::vector<std::vector<std::string>> m_cycles;
};

} // namespace sdk
} // namespace mbootcore
