#include "mbootcore/dsp/DSPDependencyGraph.hpp"
#include "mbootcore/domain/Error.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace mbootcore {
namespace dsp {

struct DSPDependencyGraph::Impl {
    std::unordered_map<std::string, DependencyNode> m_nodes;
    std::vector<std::string> m_resolutionOrder;
    bool m_resolved{false};
    bool m_hasCycle{false};
    std::vector<std::string> m_cyclePath;
    std::vector<std::string> unm_resolved;

    using AdjList = std::unordered_map<std::string, std::vector<std::string>>;

    bool dfs(const std::string& id, std::set<std::string>& visited,
             std::set<std::string>& inStack, std::vector<std::string>& path) {
        visited.insert(id);
        inStack.insert(id);
        path.push_back(id);

        const auto& deps = m_nodes[id].dependencies;
        for (const auto& dep : deps) {
            if (m_nodes.find(dep.packageId) == m_nodes.end()) continue;
            if (inStack.find(dep.packageId) != inStack.end()) {
                auto it = std::find(path.begin(), path.end(), dep.packageId);
                if (it != path.end()) {
                    m_cyclePath.assign(it, path.end());
                    m_cyclePath.push_back(dep.packageId);
                }
                return true;
            }
            if (visited.find(dep.packageId) == visited.end()) {
                if (dfs(dep.packageId, visited, inStack, path)) return true;
            }
        }

        path.pop_back();
        inStack.erase(id);
        return false;
    }

    bool detectCycle() {
        m_hasCycle = false;
        m_cyclePath.clear();
        std::set<std::string> visited;
        std::set<std::string> inStack;
        std::vector<std::string> path;

        for (const auto& [id, _] : m_nodes) {
            if (visited.find(id) == visited.end()) {
                if (dfs(id, visited, inStack, path)) {
                    m_hasCycle = true;
                    return true;
                }
            }
        }
        return false;
    }

    Result<void> topoSort() {
        unm_resolved.clear();
        m_resolutionOrder.clear();

        std::unordered_map<std::string, int> inDegree;
        for (const auto& [id, _] : m_nodes) {
            inDegree[id] = 0;
        }

        for (const auto& [id, node] : m_nodes) {
            for (const auto& dep : node.dependencies) {
                if (m_nodes.find(dep.packageId) != m_nodes.end()) {
                    inDegree[dep.packageId]++;
                }
            }
        }

        std::queue<std::string> q;
        for (const auto& [id, deg] : inDegree) {
            if (deg == 0) q.push(id);
        }

        while (!q.empty()) {
            std::string id = q.front();
            q.pop();
            m_resolutionOrder.push_back(id);

            const auto& deps = m_nodes[id].dependencies;
            for (const auto& dep : deps) {
                if (m_nodes.find(dep.packageId) != m_nodes.end()) {
                    inDegree[dep.packageId]--;
                    if (inDegree[dep.packageId] == 0) {
                        q.push(dep.packageId);
                    }
                }
            }
        }

        if (m_resolutionOrder.size() != m_nodes.size()) {
            m_hasCycle = true;
            detectCycle();
            for (const auto& [id, _] : m_nodes) {
                if (std::find(m_resolutionOrder.begin(), m_resolutionOrder.end(), id)
                    == m_resolutionOrder.end()) {
                    unm_resolved.push_back(id);
                }
            }
            m_resolved = true;
            return {};
        }

        m_resolved = true;
        m_hasCycle = false;
        return {};
    }
};

DSPDependencyGraph::DSPDependencyGraph()
    : m_impl(std::make_unique<Impl>()) {}

DSPDependencyGraph::~DSPDependencyGraph() = default;

DSPDependencyGraph::DSPDependencyGraph(DSPDependencyGraph&&) noexcept = default;
DSPDependencyGraph& DSPDependencyGraph::operator=(DSPDependencyGraph&&) noexcept = default;

Result<void> DSPDependencyGraph::addPackage(const DSPPackageMetadata& pkg) {
    const auto& id = pkg.manifest.packageId;
    if (id.empty()) {
        return ErrorCode::InvalidArgument;
    }

    if (m_impl->m_nodes.find(id) != m_impl->m_nodes.end()) {
        return ErrorCode::AlreadyExists;
    }

    DependencyNode node;
    node.packageId = id;
    node.version = pkg.manifest.version;
    node.installed = (pkg.state == DSPState::Installed);
    node.enabled = (pkg.state == DSPState::Enabled);
    node.dependencies = pkg.manifest.dependencies;

    m_impl->m_nodes[id] = std::move(node);
    m_impl->m_resolved = false;
    return {};
}

Result<void> DSPDependencyGraph::removePackage(const std::string& packageId) {
    auto it = m_impl->m_nodes.find(packageId);
    if (it == m_impl->m_nodes.end()) {
        return ErrorCode::Unknown;
    }

    for (const auto& dep : it->second.dependencies) {
        auto depIt = m_impl->m_nodes.find(dep.packageId);
        if (depIt != m_impl->m_nodes.end()) {
            auto& rb = depIt->second.requiredBy;
            rb.erase(std::remove(rb.begin(), rb.end(), packageId), rb.end());
        }
    }

    m_impl->m_nodes.erase(it);
    m_impl->m_resolved = false;
    return {};
}

Result<void> DSPDependencyGraph::resolve() {
    if (m_impl->m_nodes.empty()) {
        m_impl->m_resolutionOrder.clear();
        m_impl->m_resolved = true;
        return {};
    }

    return m_impl->topoSort();
}

void DSPDependencyGraph::clear() {
    m_impl->m_nodes.clear();
    m_impl->m_resolutionOrder.clear();
    m_impl->m_resolved = false;
    m_impl->m_hasCycle = false;
    m_impl->m_cyclePath.clear();
    m_impl->unm_resolved.clear();
}

bool DSPDependencyGraph::hasCircularDependency() const {
    if (!m_impl->m_resolved) {
        const_cast<DSPDependencyGraph*>(this)->m_impl->detectCycle();
    }
    return m_impl->m_hasCycle;
}

std::vector<std::string> DSPDependencyGraph::circularDependencyPath() const {
    if (!m_impl->m_resolved) {
        const_cast<DSPDependencyGraph*>(this)->m_impl->detectCycle();
    }
    return m_impl->m_cyclePath;
}

std::vector<std::string> DSPDependencyGraph::unresolvedDependencies() const {
    return m_impl->unm_resolved;
}

std::vector<std::string> DSPDependencyGraph::resolutionOrder() const {
    return m_impl->m_resolutionOrder;
}

const DependencyNode* DSPDependencyGraph::node(const std::string& packageId) const {
    auto it = m_impl->m_nodes.find(packageId);
    if (it == m_impl->m_nodes.end()) return nullptr;
    return &it->second;
}

std::vector<const DependencyNode*> DSPDependencyGraph::allNodes() const {
    std::vector<const DependencyNode*> result;
    result.reserve(m_impl->m_nodes.size());
    for (const auto& [_, node] : m_impl->m_nodes) {
        result.push_back(&node);
    }
    return result;
}

std::vector<const DependencyNode*> DSPDependencyGraph::roots() const {
    std::vector<const DependencyNode*> result;
    for (const auto& [id, node] : m_impl->m_nodes) {
        if (node.dependencies.empty()) {
            result.push_back(&node);
        }
    }
    return result;
}

std::vector<const DependencyNode*> DSPDependencyGraph::leaves() const {
    std::vector<const DependencyNode*> result;
    for (const auto& [id, node] : m_impl->m_nodes) {
        bool isLeaf = true;
        for (const auto& [otherId, otherNode] : m_impl->m_nodes) {
            if (&node == &otherNode) continue;
            for (const auto& dep : otherNode.dependencies) {
                if (dep.packageId == id) {
                    isLeaf = false;
                    break;
                }
            }
            if (!isLeaf) break;
        }
        if (isLeaf) result.push_back(&node);
    }
    return result;
}

size_t DSPDependencyGraph::nodeCount() const noexcept {
    return m_impl->m_nodes.size();
}

size_t DSPDependencyGraph::edgeCount() const noexcept {
    size_t count = 0;
    for (const auto& [_, node] : m_impl->m_nodes) {
        count += node.dependencies.size();
    }
    return count;
}

// --- DSPDependencyResolver ---

struct DSPDependencyResolver::Impl {
    std::vector<DSPPackageMetadata> m_installed;
    std::vector<DSPPackageMetadata> m_available;
    std::vector<std::string> m_missing;
    std::vector<std::string> m_conflicts;
    std::vector<std::string> m_obsoleted;
};

DSPDependencyResolver::DSPDependencyResolver()
    : m_impl(std::make_unique<Impl>()) {}

DSPDependencyResolver::~DSPDependencyResolver() = default;

void DSPDependencyResolver::setInstalledPackages(const std::vector<DSPPackageMetadata>& installed) {
    m_impl->m_installed = installed;
}

void DSPDependencyResolver::setAvailablePackages(const std::vector<DSPPackageMetadata>& available) {
    m_impl->m_available = available;
}

Result<std::vector<std::string>> DSPDependencyResolver::resolveInstall(const DSPPackageMetadata& pkg) {
    m_impl->m_missing.clear();
    m_impl->m_conflicts.clear();
    m_impl->m_obsoleted.clear();

    std::vector<std::string> installOrder;

    std::unordered_map<std::string, const DSPPackageMetadata*> installedMap;
    for (const auto& ip : m_impl->m_installed) {
        installedMap[ip.manifest.packageId] = &ip;
    }

    std::unordered_map<std::string, const DSPPackageMetadata*> availableMap;
    for (const auto& ap : m_impl->m_available) {
        availableMap[ap.manifest.packageId] = &ap;
    }

    std::string pkgId = pkg.manifest.packageId;

    auto it = installedMap.find(pkgId);
    if (it != installedMap.end()) {
        const auto& existing = *it->second;
        if (existing.manifest.version == pkg.manifest.version &&
            existing.state == DSPState::Installed) {
            return ErrorCode::Unknown;
        }
        if (existing.manifest.version == pkg.manifest.version &&
            existing.state == DSPState::Enabled) {
            return ErrorCode::Unknown;
        }
    }

    DSPDependencyGraph graph;
    Result<void> addResult = graph.addPackage(pkg);
    if (addResult.isError()) {
        return addResult.error();
    }

    std::vector<std::string> toProcess;
    toProcess.push_back(pkgId);

    std::set<std::string> processed;

    while (!toProcess.empty()) {
        std::string current = toProcess.back();
        toProcess.pop_back();
        if (!processed.insert(current).second) continue;

        for (const auto& dep : pkg.manifest.dependencies) {
            if (installedMap.find(dep.packageId) != installedMap.end()) {
                const auto& installedPkg = *installedMap[dep.packageId];
                if (!(dep.minVersion == DSPVersion{}) &&
                    installedPkg.manifest.version < dep.minVersion) {
                    DSPPackageMetadata updated = installedPkg;
                    updated.manifest.version = dep.minVersion;
                    m_impl->m_conflicts.push_back(dep.packageId + " installed version "
                        + installedPkg.manifest.version.toString() + " below minimum "
                        + dep.minVersion.toString());
                }
                continue;
            }

            if (availableMap.find(dep.packageId) != availableMap.end()) {
                toProcess.push_back(dep.packageId);
            } else if (dep.required) {
                m_impl->m_missing.push_back(dep.packageId);
            }
        }
    }

    if (!m_impl->m_missing.empty()) {
        return ErrorCode::Unknown;
    }

    auto resolveResult = graph.resolve();
    if (resolveResult.isError()) {
        return resolveResult.error();
    }

    installOrder = graph.resolutionOrder();

    std::unordered_set<std::string> obsoleted;
    for (const auto& ap : m_impl->m_available) {
        if (ap.manifest.packageId == pkgId) continue;
        if (installedMap.find(ap.manifest.packageId) == installedMap.end()) continue;
        bool superseded = false;
        for (const auto& dep : pkg.manifest.dependencies) {
            if (dep.packageId == ap.manifest.packageId) {
                if (!(dep.minVersion == DSPVersion{}) &&
                    ap.manifest.version < dep.minVersion) {
                    superseded = true;
                }
            }
        }
        if (superseded) {
            m_impl->m_obsoleted.push_back(ap.manifest.packageId);
            obsoleted.insert(ap.manifest.packageId);
        }
    }

    return installOrder;
}

Result<std::vector<std::string>> DSPDependencyResolver::resolveUninstall(const std::string& packageId) {
    m_impl->m_missing.clear();
    m_impl->m_conflicts.clear();
    m_impl->m_obsoleted.clear();

    std::vector<std::string> reverseDeps;

    for (const auto& ip : m_impl->m_installed) {
        for (const auto& dep : ip.manifest.dependencies) {
            if (dep.packageId == packageId && dep.required) {
                reverseDeps.push_back(ip.manifest.packageId);
            }
        }
    }

    if (!reverseDeps.empty()) {
        m_impl->m_conflicts = reverseDeps;
        return ErrorCode::Unknown;
    }

    std::vector<std::string> affected;
    affected.push_back(packageId);
    return affected;
}

Result<std::vector<std::string>> DSPDependencyResolver::resolveUpdate(const DSPPackageMetadata& newVersion) {
    m_impl->m_missing.clear();
    m_impl->m_conflicts.clear();
    m_impl->m_obsoleted.clear();

    std::string pkgId = newVersion.manifest.packageId;

    bool found = false;
    for (const auto& ip : m_impl->m_installed) {
        if (ip.manifest.packageId == pkgId) {
            found = true;
            if (newVersion.manifest.version < ip.manifest.version) {
                m_impl->m_conflicts.push_back(
                    "New version " + newVersion.manifest.version.toString()
                    + " is older than installed " + ip.manifest.version.toString());
                return ErrorCode::Unknown;
            }
            break;
        }
    }

    if (!found) {
        return ErrorCode::Unknown;
    }

    std::vector<std::string> updateOrder;
    DSPDependencyGraph graph;
    auto addResult = graph.addPackage(newVersion);
    if (addResult.isError()) {
        return addResult.error();
    }

    std::unordered_map<std::string, const DSPPackageMetadata*> availableMap;
    for (const auto& ap : m_impl->m_available) {
        availableMap[ap.manifest.packageId] = &ap;
    }

    for (const auto& dep : newVersion.manifest.dependencies) {
        bool satisfied = false;
        for (const auto& ip : m_impl->m_installed) {
            if (ip.manifest.packageId == dep.packageId) {
                if (dep.minVersion == DSPVersion{} || !(ip.manifest.version < dep.minVersion)) {
                    satisfied = true;
                } else {
                    m_impl->m_conflicts.push_back(
                        dep.packageId + " installed version too old for update");
                }
                break;
            }
        }
        if (!satisfied && dep.required) {
            if (availableMap.find(dep.packageId) != availableMap.end()) {
                m_impl->m_missing.push_back(dep.packageId);
            } else {
                m_impl->m_conflicts.push_back(
                    "Cannot satisfy dependency " + dep.packageId + " for update");
            }
        }
    }

    if (!m_impl->m_missing.empty() || !m_impl->m_conflicts.empty()) {
        return ErrorCode::Unknown;
    }

    auto resolveResult = graph.resolve();
    if (resolveResult.isError()) {
        return resolveResult.error();
    }

    updateOrder = graph.resolutionOrder();
    return updateOrder;
}

std::vector<std::string> DSPDependencyResolver::missingDependencies() const {
    return m_impl->m_missing;
}

std::vector<std::string> DSPDependencyResolver::conflictList() const {
    return m_impl->m_conflicts;
}

std::vector<std::string> DSPDependencyResolver::obsoletedPackages() const {
    return m_impl->m_obsoleted;
}

} // namespace dsp
} // namespace mbootcore
