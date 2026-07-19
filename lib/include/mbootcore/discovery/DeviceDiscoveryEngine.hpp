#pragma once

#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/domain/Error.hpp>

#include <vector>
#include <chrono>

namespace mbootcore {
namespace discovery {

class DeviceDiscoveryEngine {
public:
    explicit DeviceDiscoveryEngine(ProtocolRegistry& registry);

    DeviceDiscoveryEngine(const DeviceDiscoveryEngine&) = delete;
    DeviceDiscoveryEngine& operator=(const DeviceDiscoveryEngine&) = delete;
    DeviceDiscoveryEngine(DeviceDiscoveryEngine&&) = delete;
    DeviceDiscoveryEngine& operator=(DeviceDiscoveryEngine&&) = delete;

    DiscoveryResult discoverAll(std::chrono::milliseconds timeout = std::chrono::seconds(5));

    std::vector<DeviceDescriptor> discoverByVendor(Vendor vendor,
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

    DiscoveryResult discoverAndIdentify(std::chrono::milliseconds timeout = std::chrono::seconds(5));

    Result<DeviceDescriptor> probeDevice(const DeviceDescriptor& hint);

    void setRegistry(ProtocolRegistry& registry) { m_registry = &registry; }
    ProtocolRegistry* registry() const { return m_registry; }

private:
    ProtocolRegistry* m_registry;
};

} // namespace discovery
} // namespace mbootcore
