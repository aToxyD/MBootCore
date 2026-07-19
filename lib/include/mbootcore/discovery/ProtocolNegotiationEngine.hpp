#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/domain/Error.hpp>

#include <vector>

namespace mbootcore {
namespace discovery {

class ProtocolNegotiationEngine {
public:
    explicit ProtocolNegotiationEngine(ProtocolRegistry& registry);

    ProtocolNegotiationEngine(const ProtocolNegotiationEngine&) = delete;
    ProtocolNegotiationEngine& operator=(const ProtocolNegotiationEngine&) = delete;
    ProtocolNegotiationEngine(ProtocolNegotiationEngine&&) = delete;
    ProtocolNegotiationEngine& operator=(ProtocolNegotiationEngine&&) = delete;

    NegotiationResult negotiate(const DeviceDescriptor& descriptor);

    std::vector<NegotiationResult> negotiateAll(const DeviceDescriptor& descriptor);

    NegotiationResult bestMatch(const std::vector<NegotiationResult>& results);

    void setRegistry(ProtocolRegistry& registry) { m_registry = &registry; }
    ProtocolRegistry* registry() const { return m_registry; }

private:
    ProtocolRegistry* m_registry;
};

} // namespace discovery
} // namespace mbootcore
