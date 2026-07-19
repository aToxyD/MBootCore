#include <sdk/PluginDependencyGraph.hpp>
#include <sstream>
#include <algorithm>

namespace mbootcore {
namespace sdk {

PluginDependencyGraph::PluginDependencyGraph() = default;

void PluginDependencyGraph::addNode(const std::string& id, const std::string& type,
                                     const std::string& name, const std::string& version,
                                     const std::vector<std::string>& dependencies) {
    GraphNode node;
    node.id = id;
    node.type = type;
    node.name = name;
    node.version = version;
    node.dependsOn = dependencies;
    m_nodes[id] = node;
}

void PluginDependencyGraph::addVendorNode(const VendorRegistration& reg) {
    addNode("vendor:" + reg.name, "vendor", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addProtocolNode(const ProtocolRegistration& reg) {
    addNode("protocol:" + reg.name, "protocol", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addTransportNode(const TransportRegistration& reg) {
    addNode("transport:" + reg.name, "transport", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addWorkflowNode(const WorkflowRegistration& reg) {
    addNode("workflow:" + reg.name, "workflow", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addJobNode(const JobRegistration& reg) {
    addNode("job:" + reg.name, "job", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addPackageNode(const PackageRegistration& reg) {
    addNode("package:" + reg.name, "package", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addDiscoveryNode(const DiscoveryRegistration& reg) {
    addNode("discovery:" + reg.name, "discovery", reg.name, reg.version, reg.dependencies);
}

void PluginDependencyGraph::addCapabilityNode(const CapabilityRegistration& reg) {
    addNode("capability:" + reg.name, "capability", reg.name, reg.version, reg.dependencies);
}

bool PluginDependencyGraph::dfs(const std::string& nodeId,
                                 std::unordered_set<std::string>& visited,
                                 std::unordered_set<std::string>& inStack,
                                 std::vector<std::string>& stack) {
    if (inStack.count(nodeId)) {
        auto it = std::find(stack.begin(), stack.end(), nodeId);
        if (it != stack.end()) {
            std::vector<std::string> cycle(it, stack.end());
            cycle.push_back(nodeId);
            m_cycles.push_back(cycle);
            m_hasCycle = true;
        }
        return false;
    }
    if (visited.count(nodeId)) return true;
    if (!m_nodes.count(nodeId)) return true;

    visited.insert(nodeId);
    inStack.insert(nodeId);
    stack.push_back(nodeId);

    const auto& node = m_nodes[nodeId];
    for (const auto& dep : node.dependsOn) {
        dfs(dep, visited, inStack, stack);
    }

    stack.pop_back();
    inStack.erase(nodeId);
    return true;
}

DependencyGraphReport PluginDependencyGraph::resolve() {
    DependencyGraphReport r;
    m_hasCycle = false;
    m_cycles.clear();

    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> inStack;
    std::vector<std::string> stack;

    for (const auto& [id, node] : m_nodes) {
        if (!visited.count(id)) {
            dfs(id, visited, inStack, stack);
        }
    }

    r.hasCircularDependencies = m_hasCycle;
    r.cycles = m_cycles;
    r.nodes = m_nodes;

    for (const auto& [id, node] : m_nodes) {
        for (const auto& dep : node.dependsOn) {
            if (!m_nodes.count(dep)) {
                r.unresolvedDependencies.push_back(dep);
            }
        }
    }

    if (!m_hasCycle) {
        std::unordered_map<std::string, int> inDegree;
        for (const auto& [id, node] : m_nodes) {
            inDegree[id];
            for (const auto& dep : node.dependsOn) {
                if (m_nodes.count(dep)) {
                    inDegree[id]++;
                }
            }
        }

        std::vector<std::string> queue;
        for (const auto& [id, deg] : inDegree) {
            if (deg == 0) queue.push_back(id);
        }

        while (!queue.empty()) {
            auto id = queue.back();
            queue.pop_back();
            r.resolvedOrder.push_back(id);
            for (const auto& [nid, node] : m_nodes) {
                for (const auto& dep : node.dependsOn) {
                    if (dep == id && inDegree[nid] > 0) {
                        inDegree[nid]--;
                        if (inDegree[nid] == 0) {
                            queue.push_back(nid);
                        }
                    }
                }
            }
        }
    }

    return r;
}

std::string PluginDependencyGraph::toDot() const {
    std::ostringstream ss;
    ss << "digraph PluginDependencyGraph {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape=box, style=rounded];\n\n";

    for (const auto& [id, node] : m_nodes) {
        std::string safeId = id;
        std::replace(safeId.begin(), safeId.end(), ':', '_');
        ss << "  " << safeId << " [label=\"" << node.name << "\\n(" << node.type << ")\"];\n";
    }

    ss << "\n";
    for (const auto& [id, node] : m_nodes) {
        std::string fromId = id;
        std::replace(fromId.begin(), fromId.end(), ':', '_');
        for (const auto& dep : node.dependsOn) {
            std::string toId = dep;
            std::replace(toId.begin(), toId.end(), ':', '_');
            ss << "  " << fromId << " -> " << toId << ";\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

std::string PluginDependencyGraph::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"hasCycles\": " << (m_hasCycle ? "true" : "false") << ",\n";
    ss << "  \"nodes\": [\n";
    size_t idx = 0;
    for (const auto& [id, node] : m_nodes) {
        ss << "    {\"id\": \"" << id << "\", \"type\": \"" << node.type
           << "\", \"name\": \"" << node.name << "\"}";
        if (++idx < m_nodes.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

std::string PluginDependencyGraph::toPlantUml() const {
    std::ostringstream ss;
    ss << "@startuml\n";
    ss << "skinparam rectangle {\n";
    ss << "  roundCorner 10\n";
    ss << "}\n\n";

    for (const auto& [id, node] : m_nodes) {
        std::string safeId = id;
        std::replace(safeId.begin(), safeId.end(), ':', '_');
        std::string typeColor = "LightBlue";
        if (node.type == "vendor") typeColor = "LightGreen";
        else if (node.type == "protocol") typeColor = "LightYellow";
        else if (node.type == "transport") typeColor = "LightCoral";
        ss << "rectangle \"" << node.name << "\\n(" << node.type << ")\" as " << safeId << " #" << typeColor << "\n";
    }

    ss << "\n";
    for (const auto& [id, node] : m_nodes) {
        std::string fromId = id;
        std::replace(fromId.begin(), fromId.end(), ':', '_');
        for (const auto& dep : node.dependsOn) {
            std::string toId = dep;
            std::replace(toId.begin(), toId.end(), ':', '_');
            ss << fromId << " --> " << toId << "\n";
        }
    }

    ss << "@enduml\n";
    return ss.str();
}

void PluginDependencyGraph::clear() {
    m_nodes.clear();
    m_hasCycle = false;
    m_cycles.clear();
}

} // namespace sdk
} // namespace mbootcore
