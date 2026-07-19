#include <mbootcore/vendor/VendorPackageResolver.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareReaders.hpp>
#include <mbootcore/firmware/FirmwareValidator.hpp>

namespace mbootcore {
namespace vendor {

VendorPackageResolver::VendorPackageResolver(const VendorContext& context)
    : m_context(context) {}

Result<ResolvedPackage> VendorPackageResolver::resolvePackage(const std::string& path) {
    firmware::DirectoryFirmwareReader reader;
    auto pkgResult = reader.read(path);
    if (!pkgResult.isOk()) {
        return pkgResult.error();
    }
    ResolvedPackage resolved;
    resolved.package = std::move(pkgResult.value());
    return resolved;
}

Result<std::string> VendorPackageResolver::resolveLoader(const std::string& vendorId) {
    (void)vendorId;
    return std::string("loader.elf");
}

Result<void> VendorPackageResolver::resolveGptLayout(const std::string& vendorId) {
    (void)vendorId;
    return {};
}

Result<std::vector<std::string>> VendorPackageResolver::resolvePartitionLayout(const std::string& vendorId) {
    (void)vendorId;
    return std::vector<std::string>();
}

Result<std::vector<std::string>> VendorPackageResolver::resolveFlashingSequence(const std::string& vendorId) {
    (void)vendorId;
    return std::vector<std::string>{"programmer", "partition", "boot", "system"};
}

} // namespace vendor
} // namespace mbootcore
