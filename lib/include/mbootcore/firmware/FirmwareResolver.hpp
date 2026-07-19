#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <string>
#include <memory>

namespace mbootcore {
namespace firmware {

struct ResolvedPackage {
    std::unique_ptr<FirmwarePackage> package;
    std::string programmerPath;
    std::string gptPath;
    std::vector<std::string> resolvedDependencies;
    discovery::DeviceDescriptor targetDevice;
    bool programmerFound{false};
    bool gptFound{false};
};

class FirmwareResolver {
public:
    FirmwareResolver() = default;

    Result<ResolvedPackage> resolve(const std::string& path);
    Result<ResolvedPackage> resolve(std::unique_ptr<FirmwarePackage> pkg,
                                    const discovery::DeviceDescriptor& device = {});

    bool matchVendor(const PackageMetadata& meta,
                     const discovery::DeviceDescriptor& device) const;
    bool matchProtocol(const PackageMetadata& meta,
                       const discovery::DeviceDescriptor& device) const;
    bool matchPlatform(const PackageMetadata& meta,
                       const discovery::DeviceDescriptor& device) const;
};

} // namespace firmware
} // namespace mbootcore
