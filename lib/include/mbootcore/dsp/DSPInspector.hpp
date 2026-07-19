#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mbootcore/dsp/DSPTypes.hpp>
#include <mbootcore/dsp/DSPMetadata.hpp>
#include <mbootcore/dsp/DSPValidator.hpp>
#include <mbootcore/dsp/VendorQuirk.hpp>
#include <mbootcore/dsp/HardwareProfile.hpp>
#include <mbootcore/dsp/DSPDependencyGraph.hpp>

namespace mbootcore {
namespace dsp {

} // namespace dsp
} // namespace mbootcore

namespace mbootcore {
namespace dsp {

class DSPInspector {
public:
    DSPInspector();
    ~DSPInspector();

    Result<void> open(const std::string& packagePath);

    DSPPackageMetadata metadata() const;
    std::vector<DSPChipsetMetadata> chipsets() const;
    std::vector<DSPLoaderMetadata> loaders() const;
    std::vector<DSPBootModeMetadata> bootModes() const;
    std::vector<DSPTransportMetadata> transports() const;
    std::vector<DSPProtocolMetadata> protocols() const;
    std::vector<VendorQuirk> quirks() const;
    std::vector<HardwareProfile> profiles() const;
    std::vector<std::string> fileList() const;
    std::vector<std::string> supportedLocales() const;
    std::vector<std::string> tags() const;

    // Detailed inspection
    DSPDependencyGraph dependencyGraph() const;
    DSPCompatibilityInfo compatibilityInfo(const std::string& coreVersion) const;
    DSPPackageStatistics statistics() const;
    ValidationReport healthReport() const;

    bool isOpen() const noexcept;
    void close();
    std::string packagePath() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};



} // namespace dsp
} // namespace mbootcore
