#pragma once

#include <mbootcore/runtime/Services.hpp>
#include <mbootcore/runtime/IDeviceService.hpp>
#include <mbootcore/runtime/IFirmwareService.hpp>
#include <mbootcore/runtime/IWorkflowService.hpp>
#include <mbootcore/runtime/IPluginService.hpp>
#include <mbootcore/runtime/IDiagnosticsService.hpp>

namespace mbootcore {
namespace runtime {

// ============================================================
// RuntimeServices — concrete Services aggregator
// ============================================================

class RuntimeServices final : public Services {
public:
    RuntimeServices(IDeviceService& deviceService,
                    IFirmwareService& firmwareService,
                    IWorkflowService& workflowService,
                    IPluginService& pluginService,
                    IDiagnosticsService& diagnosticsService) noexcept
        : m_deviceService(deviceService)
        , m_firmwareService(firmwareService)
        , m_workflowService(workflowService)
        , m_pluginService(pluginService)
        , m_diagnosticsService(diagnosticsService)
    {}

    IDeviceService& devices() noexcept override { return m_deviceService; }
    IFirmwareService& firmware() noexcept override { return m_firmwareService; }
    IWorkflowService& workflows() noexcept override { return m_workflowService; }
    IPluginService& plugins() noexcept override { return m_pluginService; }
    IDiagnosticsService& diagnostics() noexcept override { return m_diagnosticsService; }

private:
    IDeviceService& m_deviceService;
    IFirmwareService& m_firmwareService;
    IWorkflowService& m_workflowService;
    IPluginService& m_pluginService;
    IDiagnosticsService& m_diagnosticsService;
};

} // namespace runtime
} // namespace mbootcore
