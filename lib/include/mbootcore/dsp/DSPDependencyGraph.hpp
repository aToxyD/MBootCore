#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

struct DependencyNode {
    std::string packageId;
    DSPVersion version;
    bool installed{false};
    bool enabled{false};
    std::vector<std::string> requiredBy;  // reverse dependency
    std::vector<DSPDependency> dependencies;
    bool resolved{false};
};

class DSPDependencyGraph {
public:
    DSPDependencyGraph();
    DSPDependencyGraph(DSPDependencyGraph&&) noexcept;
    DSPDependencyGraph& operator=(DSPDependencyGraph&&) noexcept;
    ~DSPDependencyGraph();

    Result<void> addPackage(const DSPPackageMetadata& pkg);
    Result<void> removePackage(const std::string& packageId);
    Result<void> resolve();
    void clear();

    bool hasCircularDependency() const;
    std::vector<std::string> circularDependencyPath() const;
    std::vector<std::string> unresolvedDependencies() const;
    std::vector<std::string> resolutionOrder() const;

    const DependencyNode* node(const std::string& packageId) const;
    std::vector<const DependencyNode*> allNodes() const;
    std::vector<const DependencyNode*> roots() const;
    std::vector<const DependencyNode*> leaves() const;

    size_t nodeCount() const noexcept;
    size_t edgeCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class DSPDependencyResolver {
public:
    DSPDependencyResolver();
    ~DSPDependencyResolver();

    void setInstalledPackages(const std::vector<DSPPackageMetadata>& installed);
    void setAvailablePackages(const std::vector<DSPPackageMetadata>& available);

    Result<std::vector<std::string>> resolveInstall(const DSPPackageMetadata& pkg);
    Result<std::vector<std::string>> resolveUninstall(const std::string& packageId);
    Result<std::vector<std::string>> resolveUpdate(const DSPPackageMetadata& newVersion);

    std::vector<std::string> missingDependencies() const;
    std::vector<std::string> conflictList() const;
    std::vector<std::string> obsoletedPackages() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dsp
} // namespace mbootcore
