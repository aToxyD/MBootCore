#include <mbootcore/firmware/FirmwareResolver.hpp>
#include <mbootcore/firmware/IFirmwareReader.hpp>
#include <mbootcore/firmware/FirmwareReaders.hpp>
#include <algorithm>
#include <cctype>

namespace mbootcore {
namespace firmware {

namespace {

std::string vendorToString(discovery::Vendor v) {
    switch (v) {
    case discovery::Vendor::Qualcomm:   return "QUALCOMM";
    case discovery::Vendor::MediaTek:   return "MEDIATEK";
    case discovery::Vendor::UNISOC:     return "UNISOC";
    case discovery::Vendor::Samsung:    return "SAMSUNG";
    case discovery::Vendor::Rockchip:   return "ROCKCHIP";
    case discovery::Vendor::Spreadtrum: return "SPREADTRUM";
    case discovery::Vendor::Apple:      return "APPLE";
    case discovery::Vendor::Google:     return "GOOGLE";
    case discovery::Vendor::Huawei:     return "HUAWEI";
    default:                            return "UNKNOWN";
    }
}

std::string protocolToString(discovery::ProtocolType p) {
    switch (p) {
    case discovery::ProtocolType::Sahara:        return "SAHARA";
    case discovery::ProtocolType::Firehose:      return "FIREHOSE";
    case discovery::ProtocolType::Fastboot:      return "FASTBOOT";
    case discovery::ProtocolType::MediaTekBROM:  return "MEDIATEKBROM";
    case discovery::ProtocolType::MediaTekDA:    return "MEDIATEKDA";
    case discovery::ProtocolType::UNISOCBootROM: return "UNISOCBOOTROM";
    case discovery::ProtocolType::UNISOCFDL:     return "UNISOCFDL";
    case discovery::ProtocolType::USBStream:     return "USBSTREAM";
    default:                                     return "UNKNOWN";
    }
}

} // anonymous namespace

Result<ResolvedPackage> FirmwareResolver::resolve(const std::string& path) {
    DirectoryFirmwareReader dirReader;
    if (dirReader.canRead(path)) {
        auto result = dirReader.read(path);
        if (result.isOk()) {
            return resolve(std::move(result.value()));
        }
    }

    RawFirmwareReader rawReader;
    if (rawReader.canRead(path)) {
        auto result = rawReader.read(path);
        if (result.isOk()) {
            return resolve(std::move(result.value()));
        }
    }

    return static_cast<ErrorCode>(FirmwareError::PackageNotFound);
}

Result<ResolvedPackage> FirmwareResolver::resolve(
    std::unique_ptr<FirmwarePackage> pkg,
    const discovery::DeviceDescriptor& device) {

    ResolvedPackage resolved;
    resolved.package = std::move(pkg);
    resolved.targetDevice = device;

    auto& pkgRef = *resolved.package;

    if (!matchVendor(pkgRef.metadata(), device)) {
        return static_cast<ErrorCode>(FirmwareError::UnsupportedVendor);
    }

    if (!matchProtocol(pkgRef.metadata(), device)) {
        return static_cast<ErrorCode>(FirmwareError::UnsupportedDevice);
    }

    for (const auto& dep : pkgRef.dependencies()) {
        if (dep.required) {
            resolved.resolvedDependencies.push_back(dep.id);
        }
    }

    auto progResult = pkgRef.getImageByType(ImageType::Programmer);
    if (progResult.isOk()) {
        resolved.programmerFound = true;
        resolved.programmerPath = progResult.value().info.sourceFile;
    }

    auto gptResult = pkgRef.getImageByType(ImageType::GPT);
    if (gptResult.isOk()) {
        resolved.gptFound = true;
        resolved.gptPath = gptResult.value().info.sourceFile;
    }

    return resolved;
}

bool FirmwareResolver::matchVendor(const PackageMetadata& meta,
                                    const discovery::DeviceDescriptor& device) const {
    if (meta.vendor.empty() || meta.vendor == "ANY" || meta.vendor == "GENERIC") {
        return true;
    }
    std::string pkgV = meta.vendor;
    std::transform(pkgV.begin(), pkgV.end(), pkgV.begin(), ::toupper);
    std::string devV = vendorToString(device.vendor);
    return pkgV == devV;
}

bool FirmwareResolver::matchProtocol(const PackageMetadata& meta,
                                      const discovery::DeviceDescriptor& device) const {
    if (meta.protocol.empty() || meta.protocol == "ANY" || meta.protocol == "GENERIC") {
        return true;
    }
    std::string pkgP = meta.protocol;
    std::transform(pkgP.begin(), pkgP.end(), pkgP.begin(), ::toupper);
    std::string devP = protocolToString(device.protocolHint);
    return pkgP == devP;
}

bool FirmwareResolver::matchPlatform(const PackageMetadata& meta,
                                      const discovery::DeviceDescriptor& device) const {
    if (meta.platform.empty() || meta.platform == "ANY") {
        return true;
    }
    (void)device;
    return true;
}

} // namespace firmware
} // namespace mbootcore
