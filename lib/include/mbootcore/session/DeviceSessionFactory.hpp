#pragma once

#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/domain/DeviceTypes.hpp>
#include <mbootcore/domain/Error.hpp>

#include <memory>

namespace mbootcore {
namespace discovery {
class ProtocolRegistry;
} // namespace discovery

namespace session {

class DeviceSessionFactory {
public:
    explicit DeviceSessionFactory(discovery::ProtocolRegistry& registry);

    DeviceSessionFactory(const DeviceSessionFactory&) = delete;
    DeviceSessionFactory& operator=(const DeviceSessionFactory&) = delete;
    DeviceSessionFactory(DeviceSessionFactory&&) = delete;
    DeviceSessionFactory& operator=(DeviceSessionFactory&&) = delete;

    Result<std::unique_ptr<DeviceSession>> createSession(const discovery::DeviceDescriptor& descriptor);

    Result<std::unique_ptr<DeviceSession>> createSessionWithDiscovery(
        const discovery::DeviceDescriptor& hint);

    discovery::ProtocolRegistry& registry() noexcept { return *m_registry; }

private:
    discovery::ProtocolRegistry* m_registry;
};

} // namespace session
} // namespace mbootcore
