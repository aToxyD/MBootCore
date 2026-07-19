#pragma once

#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/session/DeviceSessionFactory.hpp>
#include <mbootcore/domain/DeviceTypes.hpp>

#include <memory>
#include <shared_mutex>
#include <vector>
#include <string>

namespace mbootcore {
namespace discovery {
class ProtocolRegistry;
} // namespace discovery

namespace session {

class DeviceManager {
public:
    explicit DeviceManager(discovery::ProtocolRegistry& registry);

    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    DeviceManager(DeviceManager&&) = delete;
    DeviceManager& operator=(DeviceManager&&) = delete;

    // Session lifecycle
    Result<DeviceSession*> createSession(const discovery::DeviceDescriptor& descriptor);
    Result<void> removeSession(const std::string& sessionId);
    Result<void> removeSession(DeviceSession* session);
    void removeAll();

    // Lookup
    DeviceSession* findById(const std::string& sessionId) const;
    DeviceSession* findByDescriptor(const discovery::DeviceDescriptor& descriptor) const;
    std::vector<DeviceSession*> findByVendor(discovery::Vendor vendor) const;
    std::vector<DeviceSession*> findByProtocol(discovery::ProtocolType protocol) const;
    std::vector<DeviceSession*> findByState(SessionState state) const;

    // Bulk operations
    std::vector<DeviceSession*> activeSessions() const;
    std::vector<DeviceSession*> allSessions() const;
    std::size_t sessionCount() const;
    std::size_t activeCount() const;

    // Connect/disconnect all
    Result<void> connectAll();
    void disconnectAll();
    Result<void> reconnectAll();

    // Discovery integration
    Result<std::vector<DeviceSession*>> discoverAndCreate(
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

    // Factory
    DeviceSessionFactory& factory() noexcept { return m_factory; }
    discovery::ProtocolRegistry& registry() noexcept { return *m_registry; }

private:
    mutable std::shared_mutex m_sessionsMutex;
    discovery::ProtocolRegistry* m_registry;
    DeviceSessionFactory m_factory;
    std::vector<std::unique_ptr<DeviceSession>> m_sessions;
};

} // namespace session
} // namespace mbootcore
