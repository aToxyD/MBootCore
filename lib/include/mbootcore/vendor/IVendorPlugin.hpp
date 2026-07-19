#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore {
namespace vendor {

class IVendorPlugin {
public:
    virtual ~IVendorPlugin() = default;
    virtual Result<void> registerVendor(const VendorContext& context) = 0;
    virtual Result<void> unregisterVendor() noexcept = 0;
    virtual VendorInfo vendorInfo() const = 0;
    virtual VendorCapability capabilities() const = 0;
    virtual std::string_view name() const noexcept = 0;
};

} // namespace vendor
} // namespace mbootcore
