#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>

#include <memory>

namespace mbootcore {
namespace discovery {

class IProtocolFactory {
public:
    virtual ~IProtocolFactory() = default;

    virtual ProtocolType protocolType() const = 0;

    virtual std::unique_ptr<IFlashDevice> createFlashDevice(const DeviceDescriptor& descriptor) = 0;

    virtual std::unique_ptr<pipeline::BootPipeline> createPipeline(const DeviceDescriptor& descriptor) = 0;
};

} // namespace discovery
} // namespace mbootcore
