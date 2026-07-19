#include <mbootcore/vendor/VendorFactory.hpp>
#include <mbootcore/vendor/IVendorPlugin.hpp>
#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <algorithm>
#include <string>

namespace mbootcore {
namespace vendor {

namespace {

std::string protocolTypeToString(discovery::ProtocolType pt) {
    switch (pt) {
        case discovery::ProtocolType::Sahara: return "Sahara";
        case discovery::ProtocolType::Firehose: return "Firehose";
        case discovery::ProtocolType::MediaTekBROM: return "MediaTekBROM";
        case discovery::ProtocolType::MediaTekDA: return "MediaTekDA";
        case discovery::ProtocolType::UNISOCBootROM: return "UNISOCBootROM";
        case discovery::ProtocolType::UNISOCFDL: return "UNISOCFDL";
        default: return "";
    }
}

std::string vendorToId(discovery::Vendor v) {
    switch (v) {
        case discovery::Vendor::Qualcomm: return "qualcomm";
        case discovery::Vendor::MediaTek: return "mediatek";
        case discovery::Vendor::UNISOC: return "unisoc";
        case discovery::Vendor::Rockchip: return "rockchip";
        case discovery::Vendor::Samsung: return "samsung";
        default: return "";
    }
}

class NullVendor : public IVendor {
public:
    Result<void> initialize(const VendorContext&) override { return {}; }
    Result<void> shutdown() noexcept override { return {}; }
    VendorInfo vendorInfo() const override {
        VendorInfo info;
        info.id = "null";
        info.name = "Null Vendor";
        info.family = VendorFamily::Unknown;
        return info;
    }
    VendorCapability capabilities() const override { return VendorCapability::None; }
    std::unique_ptr<discovery::IDeviceDetector> createDetector() override { return {}; }
    std::unique_ptr<discovery::IProtocolNegotiator> createNegotiator() override { return {}; }
    std::unique_ptr<pipeline::BootPipeline> createPipeline() override { return {}; }
    std::unique_ptr<IFlashDevice> createFlashDevice() override { return {}; }
    std::unique_ptr<workflow::WorkflowEngine> createWorkflow() override { return {}; }
    std::string_view name() const noexcept override { return "null"; }
    std::unique_ptr<IVendor> clone() const override { return std::make_unique<NullVendor>(); }
};
}

VendorRegistry& VendorFactory::registry() {
    static VendorRegistry instance;
    return instance;
}

std::unique_ptr<IVendor> VendorFactory::createVendor(VendorFamily family) {
    auto* vendor = registry().resolveByFamily(family);
    return vendor ? vendor->clone() : nullptr;
}

std::unique_ptr<IVendor> VendorFactory::createVendor(const std::string& vendorId) {
    auto* vendor = registry().resolveById(vendorId);
    return vendor ? vendor->clone() : nullptr;
}

std::unique_ptr<IVendorPlugin> VendorFactory::createPlugin(const std::string& pluginId) {
    (void)pluginId;
    return nullptr;
}

std::unique_ptr<IVendor> VendorFactory::createFromDescriptor(const discovery::DeviceDescriptor& descriptor) {
    auto protoStr = protocolTypeToString(descriptor.protocolHint);
    if (!protoStr.empty()) {
        auto vendors = registry().resolveByProtocol(protoStr);
        if (!vendors.empty()) return vendors.front()->clone();
    }
    auto vid = vendorToId(descriptor.vendor);
    if (!vid.empty()) {
        auto* vendor = registry().resolveById(vid);
        if (vendor) return vendor->clone();
    }
    return nullptr;
}

std::unique_ptr<IVendor> VendorFactory::createFromDiscovery(const discovery::DiscoveryResult& discovery) {
    for (const auto& desc : discovery.devices) {
        auto result = createFromDescriptor(desc);
        if (result) return result;
    }
    return nullptr;
}

std::unique_ptr<IVendor> VendorFactory::createFromSession(const session::SessionConfig& config) {
    (void)config;
    return nullptr;
}

Result<void> VendorFactory::registerVendor(std::unique_ptr<IVendor> vendor) {
    return registry().registerVendor(std::move(vendor));
}

void VendorFactory::clearRegistry() {
    registry().clear();
}

} // namespace vendor
} // namespace mbootcore
