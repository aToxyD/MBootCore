#pragma once

#include <mbootcore/runtime/IDiagnosticsService.hpp>

#include <mbootcore/transport/TransportManager.hpp>

#include <memory>
#include <mutex>

namespace mbootcore {
namespace runtime {

struct RuntimeState;
class DeviceService;
class PluginService;

class DiagnosticsService final : public IDiagnosticsService {
public:
    DiagnosticsService(RuntimeState& state,
                       transport::TransportManager& transportManager,
                       DeviceService& deviceService,
                       PluginService& pluginService) noexcept
        : m_state(state)
        , m_transportManager(transportManager)
        , m_deviceService(deviceService)
        , m_pluginService(pluginService)
    {}

    DiagnosticsService(const DiagnosticsService&) = delete;
    DiagnosticsService& operator=(const DiagnosticsService&) = delete;
    DiagnosticsService(DiagnosticsService&&) noexcept = delete;
    DiagnosticsService& operator=(DiagnosticsService&&) noexcept = delete;

    RuntimeStatistics statistics() const override;
    RuntimeHealth health() const override;
    HardwareDiagnosticReport hardwareReport() const override;
    std::vector<HardwareDeviceEntry> connectedHardware() const override;
    HardwareTransportStats transportStatistics(transport::TransportType type) const override;
    std::vector<transport::usb::UsbDeviceInfo> usbDevices() const override;
    std::vector<std::string> serialPorts() const override;

    // Non-virtual helpers (used by hardwareReport)
    std::string versionString() const;

private:
    RuntimeState& m_state;
    transport::TransportManager& m_transportManager;
    DeviceService& m_deviceService;
    PluginService& m_pluginService;
};

} // namespace runtime
} // namespace mbootcore
