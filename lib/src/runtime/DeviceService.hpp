#pragma once

#include <mbootcore/runtime/IDeviceService.hpp>

#include <mbootcore/discovery/DeviceDiscoveryEngine.hpp>
#include <mbootcore/discovery/ProtocolNegotiationEngine.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/session/DeviceManager.hpp>
#include <mbootcore/session/DeviceSession.hpp>

#include <memory>
#include <atomic>

namespace mbootcore {
namespace runtime {

class DeviceService final : public IDeviceService {
public:
    DeviceService(
        std::unique_ptr<discovery::ProtocolRegistry> registry,
        std::unique_ptr<discovery::DeviceDiscoveryEngine> discoveryEngine,
        std::unique_ptr<discovery::ProtocolNegotiationEngine> negotiationEngine,
        std::unique_ptr<session::DeviceManager> sessionManager);

    ~DeviceService() override = default;

    DeviceService(const DeviceService&) = delete;
    DeviceService& operator=(const DeviceService&) = delete;
    DeviceService(DeviceService&&) noexcept = default;
    DeviceService& operator=(DeviceService&&) noexcept = default;

    discovery::ProtocolRegistry& protocolRegistry() noexcept { return *m_protocolRegistry; }
    discovery::DeviceDiscoveryEngine& discoveryEngine() noexcept { return *m_discoveryEngine; }
    discovery::ProtocolNegotiationEngine& negotiationEngine() noexcept { return *m_negotiationEngine; }
    session::DeviceManager& sessionManager() noexcept { return *m_sessionManager; }

    Result<std::vector<discovery::DeviceDescriptor>> discover(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)) override;

    Result<discovery::DeviceDescriptor> probe(
        const discovery::DeviceDescriptor& hint) override;

    Result<void> connect(const discovery::DeviceDescriptor& descriptor) override;
    void disconnect() noexcept override;
    Result<void> reconnect() override;
    bool isConnected() const noexcept override { return m_activeSession.load() != nullptr; }
    session::DeviceSession* activeSession() const noexcept override { return m_activeSession.load(); }

private:
    std::unique_ptr<discovery::ProtocolRegistry> m_protocolRegistry;
    std::unique_ptr<discovery::DeviceDiscoveryEngine> m_discoveryEngine;
    std::unique_ptr<discovery::ProtocolNegotiationEngine> m_negotiationEngine;
    std::unique_ptr<session::DeviceManager> m_sessionManager;
    std::atomic<session::DeviceSession*> m_activeSession{nullptr};
};

} // namespace runtime
} // namespace mbootcore
