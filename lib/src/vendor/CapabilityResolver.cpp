#include <mbootcore/vendor/CapabilityResolver.hpp>
#include <algorithm>

namespace mbootcore {
namespace vendor {

CapabilityResolver::CapabilityResolver(const VendorRegistry& registry)
    : m_registry(registry) {}

bool CapabilityResolver::canResolve(const std::string& vendorId, VendorCapability cap) const {
    auto* vendor = m_registry.resolveById(vendorId);
    if (!vendor) return false;
    return hasCapability(vendor->capabilities(), cap);
}

std::vector<VendorCapability> CapabilityResolver::resolveForVendor(const std::string& vendorId) const {
    std::vector<VendorCapability> result;
    auto* vendor = m_registry.resolveById(vendorId);
    if (!vendor) return result;
    auto caps = vendor->capabilities();
    auto all = {
        VendorCapability::BootROM, VendorCapability::DownloadAgent,
        VendorCapability::LoaderUpload, VendorCapability::MemoryRead,
        VendorCapability::MemoryWrite, VendorCapability::Flash,
        VendorCapability::GPT, VendorCapability::Partition,
        VendorCapability::SecureBoot, VendorCapability::Authentication,
        VendorCapability::Reset, VendorCapability::Reboot,
        VendorCapability::Verify, VendorCapability::Streaming,
        VendorCapability::Logging
    };
    for (auto c : all) {
        if (hasCapability(caps, c)) result.push_back(c);
    }
    return result;
}

std::vector<IVendor*> CapabilityResolver::resolveVendorsByCapability(VendorCapability cap) const {
    return m_registry.resolveByCapability(cap);
}

VendorCapability CapabilityResolver::resolveCapabilities(const std::string& vendorId) const {
    auto* vendor = m_registry.resolveById(vendorId);
    return vendor ? vendor->capabilities() : VendorCapability::None;
}

bool CapabilityResolver::hasFallback(const std::string& vendorId, VendorCapability cap) const {
    (void)vendorId;
    (void)cap;
    return false;
}

std::vector<VendorCapability> CapabilityResolver::resolveOptional(const std::string& vendorId) const {
    (void)vendorId;
    return {};
}

} // namespace vendor
} // namespace mbootcore
