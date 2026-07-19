#pragma once

#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/discovery/DeviceDiscoveryEngine.hpp>
#include <mbootcore/discovery/ProtocolNegotiationEngine.hpp>
#include <mbootcore/session/DeviceManager.hpp>
#include <mbootcore/domain/ILogger.hpp>

#include <string>
#include <memory>

namespace mbootcore {
namespace plugin {

class PluginContext {
public:
    PluginContext(discovery::ProtocolRegistry& registry,
                  session::DeviceManager* deviceManager = nullptr,
                  ILogger* logger = nullptr)
        : m_registry(&registry)
        , m_discoveryEngine(std::make_unique<discovery::DeviceDiscoveryEngine>(registry))
        , m_negotiationEngine(std::make_unique<discovery::ProtocolNegotiationEngine>(registry))
        , m_deviceManager(deviceManager)
        , m_logger(logger) {}

    discovery::ProtocolRegistry& registry() const noexcept { return *m_registry; }
    discovery::DeviceDiscoveryEngine& discoveryEngine() const noexcept { return *m_discoveryEngine; }
    discovery::ProtocolNegotiationEngine& negotiationEngine() const noexcept { return *m_negotiationEngine; }
    session::DeviceManager* deviceManager() const noexcept { return m_deviceManager; }
    ILogger* logger() const noexcept { return m_logger; }

    void setDeviceManager(session::DeviceManager* dm) noexcept { m_deviceManager = dm; }
    void setLogger(ILogger* logger) noexcept { m_logger = logger; }

    void log(const std::string& message) {
        if (m_logger) m_logger->debug("Plugin", message);
    }

private:
    discovery::ProtocolRegistry* m_registry;
    std::unique_ptr<discovery::DeviceDiscoveryEngine> m_discoveryEngine;
    std::unique_ptr<discovery::ProtocolNegotiationEngine> m_negotiationEngine;
    session::DeviceManager* m_deviceManager{nullptr};
    ILogger* m_logger{nullptr};
};

} // namespace plugin
} // namespace mbootcore
