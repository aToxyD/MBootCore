#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

class DSPRepository;

class DSPManager {
public:
    explicit DSPManager(std::unique_ptr<DSPRepository> repository);

    // Lifecycle
    Result<void> install(const std::string& packagePath);
    Result<void> uninstall(const std::string& packageId);
    Result<void> update(const std::string& packageId, const std::string& packagePath);
    Result<void> enable(const std::string& packageId);
    Result<void> disable(const std::string& packageId);
    Result<void> repair(const std::string& packageId);
    Result<void> verify(const std::string& packageId, DSPValidationLevel level = DSPValidationLevel::Full);
    Result<void> reload();

    // Queries
    std::vector<DSPPackageStatistics> listInstalled() const;
    std::vector<DSPPackageStatistics> listAvailable() const;
    const DSPPackageMetadata* findPackage(const std::string& packageId) const;
    std::vector<const DSPPackageMetadata*> findVendor(discovery::Vendor vendor) const;
    std::vector<const DSPChipsetMetadata*> findChipset(const ChipsetId& id) const;
    std::vector<const DSPLoaderMetadata*> findLoader(const std::string& loaderName) const;
    std::vector<const DSPLoaderMetadata*> findLoadersForChipset(const ChipsetId& id) const;
    std::vector<const DSPLoaderMetadata*> findLoadersForProtocol(discovery::ProtocolType proto) const;

    // Stats & health
    DSPPackageStatistics statistics(const std::string& packageId) const;
    std::vector<std::string> healthReport(const std::string& packageId) const;

    // Access
    DSPRepository& repository() noexcept { return *m_repository; }
    const DSPRepository& repository() const noexcept { return *m_repository; }

    // Counts
    size_t installedCount() const noexcept;
    size_t enabledCount() const noexcept;
    size_t totalLoaderCount() const noexcept;
    size_t totalChipsetCount() const noexcept;

    using ProgressCallback = std::function<void(const std::string& operation, int percent)>;
    void setProgressCallback(ProgressCallback cb);

private:
    std::unique_ptr<DSPRepository> m_repository;
    std::unordered_map<std::string, DSPPackageMetadata> m_packages;
    ProgressCallback m_progressCb;

    void reportProgress(const std::string& op, int pct);
    DSPPackageStatistics buildStatistics(const DSPPackageMetadata& pkg) const;
    Result<void> loadPackageManifest(const std::string& packagePath, DSPPackageMetadata& meta);
    bool hasLoaderConflict(const DSPLoaderMetadata& loader) const;
};

} // namespace dsp
} // namespace mbootcore
