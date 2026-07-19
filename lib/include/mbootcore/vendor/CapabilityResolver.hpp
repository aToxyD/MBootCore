#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/domain/Error.hpp>
#include <vector>
#include <string>

namespace mbootcore {
namespace vendor {

class CapabilityResolver {
public:
    explicit CapabilityResolver(const VendorRegistry& registry);
    CapabilityResolver(const CapabilityResolver&) = default;
    CapabilityResolver& operator=(const CapabilityResolver&) = delete;

    bool canResolve(const std::string& vendorId, VendorCapability cap) const;
    std::vector<VendorCapability> resolveForVendor(const std::string& vendorId) const;
    std::vector<IVendor*> resolveVendorsByCapability(VendorCapability cap) const;
    VendorCapability resolveCapabilities(const std::string& vendorId) const;
    bool hasFallback(const std::string& vendorId, VendorCapability cap) const;
    std::vector<VendorCapability> resolveOptional(const std::string& vendorId) const;

private:
    const VendorRegistry& m_registry;
};

} // namespace vendor
} // namespace mbootcore
