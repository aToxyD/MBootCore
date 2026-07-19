#include <mbootcore/session/DeviceManager.hpp>
#include <mbootcore/discovery/DeviceDiscoveryEngine.hpp>

namespace mbootcore {
namespace session {

DeviceManager::DeviceManager(discovery::ProtocolRegistry& registry)
    : m_registry(&registry), m_factory(registry) {}

Result<DeviceSession*> DeviceManager::createSession(const discovery::DeviceDescriptor& descriptor) {
    auto sessionResult = m_factory.createSession(descriptor);
    if (sessionResult.isError()) {
        return sessionResult.error();
    }

    auto* session = sessionResult.value().get();
    {
        std::unique_lock lock(m_sessionsMutex);
        m_sessions.push_back(std::move(sessionResult.value()));
    }
    return session;
}

Result<void> DeviceManager::removeSession(const std::string& sessionId) {
    std::unique_ptr<DeviceSession> target;
    {
        std::unique_lock lock(m_sessionsMutex);
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if ((*it)->sessionId() == sessionId) {
                target = std::move(*it);
                m_sessions.erase(it);
                break;
            }
        }
    }
    if (!target) return ErrorCode::DeviceNotFound;
    target->disconnect();
    return {};
}

Result<void> DeviceManager::removeSession(DeviceSession* session) {
    if (!session) return ErrorCode::InvalidArgument;
    std::unique_ptr<DeviceSession> target;
    {
        std::unique_lock lock(m_sessionsMutex);
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (it->get() == session) {
                target = std::move(*it);
                m_sessions.erase(it);
                break;
            }
        }
    }
    if (!target) return ErrorCode::DeviceNotFound;
    target->disconnect();
    return {};
}

void DeviceManager::removeAll() {
    std::vector<std::unique_ptr<DeviceSession>> all;
    {
        std::unique_lock lock(m_sessionsMutex);
        all = std::move(m_sessions);
        m_sessions.clear();
    }
    for (auto& session : all) {
        session->disconnect();
    }
}

DeviceSession* DeviceManager::findById(const std::string& sessionId) const {
    std::shared_lock lock(m_sessionsMutex);
    for (const auto& s : m_sessions) {
        if (s->sessionId() == sessionId) return s.get();
    }
    return nullptr;
}

DeviceSession* DeviceManager::findByDescriptor(const discovery::DeviceDescriptor& descriptor) const {
    std::shared_lock lock(m_sessionsMutex);
    for (const auto& s : m_sessions) {
        const auto& d = s->descriptor();
        if (descriptor.isUsb() && d.isUsb() && descriptor.usbVid == d.usbVid && descriptor.usbPid == d.usbPid) {
            return s.get();
        }
        if (!descriptor.connectionPath.empty() && descriptor.connectionPath == d.connectionPath) {
            return s.get();
        }
    }
    return nullptr;
}

std::vector<DeviceSession*> DeviceManager::findByVendor(discovery::Vendor vendor) const {
    std::shared_lock lock(m_sessionsMutex);
    std::vector<DeviceSession*> result;
    result.reserve(m_sessions.size());
    for (const auto& s : m_sessions) {
        if (s->descriptor().vendor == vendor) {
            result.push_back(s.get());
        }
    }
    return result;
}

std::vector<DeviceSession*> DeviceManager::findByProtocol(discovery::ProtocolType protocol) const {
    std::shared_lock lock(m_sessionsMutex);
    std::vector<DeviceSession*> result;
    result.reserve(m_sessions.size());
    for (const auto& s : m_sessions) {
        if (s->descriptor().protocolHint == protocol) {
            result.push_back(s.get());
        }
    }
    return result;
}

std::vector<DeviceSession*> DeviceManager::findByState(SessionState state) const {
    std::shared_lock lock(m_sessionsMutex);
    std::vector<DeviceSession*> result;
    result.reserve(m_sessions.size());
    for (const auto& s : m_sessions) {
        if (s->state() == state) {
            result.push_back(s.get());
        }
    }
    return result;
}

std::vector<DeviceSession*> DeviceManager::activeSessions() const {
    std::shared_lock lock(m_sessionsMutex);
    std::vector<DeviceSession*> result;
    result.reserve(m_sessions.size());
    for (const auto& s : m_sessions) {
        if (s->isConnected()) {
            result.push_back(s.get());
        }
    }
    return result;
}

std::vector<DeviceSession*> DeviceManager::allSessions() const {
    std::shared_lock lock(m_sessionsMutex);
    std::vector<DeviceSession*> result;
    result.reserve(m_sessions.size());
    for (const auto& s : m_sessions) {
        result.push_back(s.get());
    }
    return result;
}

std::size_t DeviceManager::activeCount() const {
    std::shared_lock lock(m_sessionsMutex);
    std::size_t count = 0;
    for (const auto& s : m_sessions) {
        if (s->isConnected()) ++count;
    }
    return count;
}

std::size_t DeviceManager::sessionCount() const {
    std::shared_lock lock(m_sessionsMutex);
    return m_sessions.size();
}

Result<void> DeviceManager::connectAll() {
    std::vector<DeviceSession*> sessions;
    {
        std::unique_lock lock(m_sessionsMutex);
        sessions.reserve(m_sessions.size());
        for (auto& s : m_sessions) {
            sessions.push_back(s.get());
        }
    }
    for (auto* s : sessions) {
        auto result = s->connect();
        if (result.isError()) return result;
    }
    return {};
}

void DeviceManager::disconnectAll() {
    std::vector<DeviceSession*> sessions;
    {
        std::unique_lock lock(m_sessionsMutex);
        sessions.reserve(m_sessions.size());
        for (auto& s : m_sessions) {
            sessions.push_back(s.get());
        }
    }
    for (auto* s : sessions) {
        s->disconnect();
    }
}

Result<void> DeviceManager::reconnectAll() {
    std::vector<DeviceSession*> sessions;
    {
        std::unique_lock lock(m_sessionsMutex);
        sessions.reserve(m_sessions.size());
        for (auto& s : m_sessions) {
            sessions.push_back(s.get());
        }
    }
    for (auto* s : sessions) {
        s->disconnect();
    }
    for (auto* s : sessions) {
        auto result = s->connect();
        if (result.isError()) return result;
    }
    return {};
}

Result<std::vector<DeviceSession*>> DeviceManager::discoverAndCreate(
    std::chrono::milliseconds timeout)
{
    discovery::DeviceDiscoveryEngine engine(*m_registry);
    auto discoveryResult = engine.discoverAll(timeout);

    std::vector<DeviceSession*> created;
    for (const auto& desc : discoveryResult.devices) {
        auto sessionResult = m_factory.createSession(desc);
        if (sessionResult.isOk()) {
            auto* session = sessionResult.value().get();
            {
                std::unique_lock lock(m_sessionsMutex);
                m_sessions.push_back(std::move(sessionResult.value()));
            }
            created.push_back(session);
        }
    }

    return created;
}

} // namespace session
} // namespace mbootcore
