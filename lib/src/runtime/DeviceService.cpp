#include "DeviceService.hpp"

namespace mbootcore {
namespace runtime {

DeviceService::DeviceService(
    std::unique_ptr<discovery::ProtocolRegistry> registry,
    std::unique_ptr<discovery::DeviceDiscoveryEngine> discoveryEngine,
    std::unique_ptr<discovery::ProtocolNegotiationEngine> negotiationEngine,
    std::unique_ptr<session::DeviceManager> sessionManager)
    : m_protocolRegistry(std::move(registry))
    , m_discoveryEngine(std::move(discoveryEngine))
    , m_negotiationEngine(std::move(negotiationEngine))
    , m_sessionManager(std::move(sessionManager))
{
}

Result<std::vector<discovery::DeviceDescriptor>> DeviceService::discover(std::chrono::milliseconds timeout) {
    auto result = m_discoveryEngine->discoverAll(timeout);
    if (result.error) {
        return ErrorCode::EnumerationFailed;
    }
    return result.devices;
}

Result<discovery::DeviceDescriptor> DeviceService::probe(const discovery::DeviceDescriptor& hint) {
    return m_discoveryEngine->probeDevice(hint);
}

Result<void> DeviceService::connect(const discovery::DeviceDescriptor& descriptor) {
    MBOOT_TRY_ASSIGN(sessionPtr, m_sessionManager->createSession(descriptor));
    MBOOT_TRY(sessionPtr->connect());

    m_activeSession.store(sessionPtr);
    return {};
}

void DeviceService::disconnect() noexcept {
    auto* session = m_activeSession.exchange(nullptr);
    if (session) session->disconnect();
    m_sessionManager->removeAll();
}

Result<void> DeviceService::reconnect() {
    auto* session = m_activeSession.load();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->reconnect();
}

} // namespace runtime
} // namespace mbootcore
