#include "mbootcore/session/DeviceSessionFactory.hpp"

#include <mbootcore/discovery/DeviceDiscoveryEngine.hpp>
#include <mbootcore/discovery/ProtocolNegotiationEngine.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/pipeline/BootPipelineFactory.hpp>

namespace mbootcore {
namespace session {

DeviceSessionFactory::DeviceSessionFactory(discovery::ProtocolRegistry& registry)
    : m_registry(&registry) {}

Result<std::unique_ptr<DeviceSession>> DeviceSessionFactory::createSession(
    const discovery::DeviceDescriptor& descriptor)
{
    auto descriptorCopy = descriptor;

    // Resolve protocol hint via negotiation if not already set
    if (descriptorCopy.protocolHint == discovery::ProtocolType::Unknown && descriptor.isValid()) {
        discovery::ProtocolNegotiationEngine negotiator(*m_registry);
        auto negotiationResult = negotiator.negotiate(descriptorCopy);
        if (negotiationResult.protocol != discovery::ProtocolType::Unknown) {
            descriptorCopy.protocolHint = negotiationResult.protocol;
        }
    }

    // Find factory for the protocol
    auto* factory = m_registry->findFactory(descriptorCopy.protocolHint);
    if (!factory) {
        return ErrorCode::NoMatchingProtocol;
    }

    // Create flash device
    auto flashDevice = factory->createFlashDevice(descriptorCopy);
    if (!flashDevice) {
        return ErrorCode::NotSupported;
    }

    // Create pipeline from descriptor
    auto pipeline = pipeline::BootPipelineFactory::createFromDescriptor(descriptorCopy);

    auto session = std::make_unique<DeviceSession>(
        std::move(descriptorCopy), std::move(flashDevice), std::move(pipeline));

    return session;
}

Result<std::unique_ptr<DeviceSession>> DeviceSessionFactory::createSessionWithDiscovery(
    const discovery::DeviceDescriptor& hint)
{
    // Probe the device
    discovery::DeviceDiscoveryEngine discoveryEngine(*m_registry);
    auto probeResult = discoveryEngine.probeDevice(hint);
    if (probeResult.isError()) {
        return probeResult.error();
    }

    // Create session from probed descriptor
    return createSession(probeResult.value());
}

} // namespace session
} // namespace mbootcore
