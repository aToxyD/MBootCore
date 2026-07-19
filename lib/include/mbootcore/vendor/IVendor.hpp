#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/domain/Error.hpp>
#include <memory>

namespace mbootcore {

namespace discovery { class IDeviceDetector; }
namespace discovery { class IProtocolNegotiator; }
namespace pipeline { class BootPipeline; }
class IFlashDevice;
namespace workflow { class WorkflowEngine; }

namespace vendor {

class IVendor {
public:
    virtual ~IVendor() = default;
    virtual Result<void> initialize(const VendorContext& context) = 0;
    virtual Result<void> shutdown() noexcept = 0;
    virtual VendorInfo vendorInfo() const = 0;
    virtual VendorCapability capabilities() const = 0;
    virtual std::unique_ptr<discovery::IDeviceDetector> createDetector() = 0;
    virtual std::unique_ptr<discovery::IProtocolNegotiator> createNegotiator() = 0;
    virtual std::unique_ptr<pipeline::BootPipeline> createPipeline() = 0;
    virtual std::unique_ptr<IFlashDevice> createFlashDevice() = 0;
    virtual std::unique_ptr<workflow::WorkflowEngine> createWorkflow() = 0;
    virtual std::string_view name() const noexcept = 0;
    virtual std::unique_ptr<IVendor> clone() const = 0;
};

} // namespace vendor
} // namespace mbootcore
