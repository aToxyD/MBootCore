#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>

namespace mbootcore {
namespace dsp {

// Repository search query
struct DSPQuery {
    std::string searchText;
    discovery::Vendor vendor{discovery::Vendor::Unknown};
    ChipsetId chipset;
    std::string loaderName;
    discovery::BootMode bootMode{discovery::BootMode::Unknown};
    discovery::ProtocolType protocol{discovery::ProtocolType::Unknown};
    DSPVersion minVersion;
    DSPVersion maxVersion;
    std::string capability;
    std::string tag;
    DSPState state{DSPState::Unknown};
    DSPOrigin origin{DSPOrigin::Unknown};
    bool caseSensitive{false};
};

// Repository type
enum class RepositoryType : uint32_t {
    Installed = 0,
    System    = 1,
    User      = 2,
    Portable  = 3,
    Online    = 4
};

class DSPRepository {
public:
    DSPRepository();
    ~DSPRepository();

    // Repository paths
    void setSystemPath(const std::string& path);
    void setUserPath(const std::string& path);
    void setPortablePath(const std::string& path);
    std::string systemPath() const noexcept;
    std::string userPath() const noexcept;
    std::string portablePath() const noexcept;

    // Scanning
    Result<void> scan();
    Result<void> scanPath(const std::string& path);

    // Queries by search
    std::vector<DSPPackageStatistics> listAll() const;
    std::vector<DSPPackageStatistics> search(const DSPQuery& query) const;

    // Package lookup
    const DSPPackageMetadata* byId(const std::string& packageId) const;
    std::vector<const DSPPackageMetadata*> byVendor(discovery::Vendor vendor) const;
    std::vector<const DSPPackageMetadata*> byChipset(const ChipsetId& id) const;
    std::vector<const DSPPackageMetadata*> byProtocol(discovery::ProtocolType proto) const;
    std::vector<const DSPPackageMetadata*> byBootMode(discovery::BootMode mode) const;
    std::vector<const DSPPackageMetadata*> byLoader(const std::string& loaderName) const;
    std::vector<const DSPPackageMetadata*> byOrigin(DSPOrigin origin) const;

    // Direct access to contained metadata
    std::vector<const DSPChipsetMetadata*> allChipsets() const;
    std::vector<const DSPLoaderMetadata*> allLoaders() const;
    std::vector<const DSPBootModeMetadata*> allBootModes() const;
    std::vector<const DSPTransportMetadata*> allTransports() const;
    std::vector<const DSPProtocolMetadata*> allProtocols() const;
    std::vector<const DSPVendorMetadata*> allVendors() const;

    // Repository management
    size_t packageCount() const noexcept;
    bool isEmpty() const noexcept;
    void clear();

    // Save/load index
    Result<void> saveIndex();
    Result<void> loadIndex();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dsp
} // namespace mbootcore
