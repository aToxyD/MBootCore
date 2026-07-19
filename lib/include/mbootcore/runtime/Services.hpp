#pragma once

#include <mbootcore/runtime/IDeviceService.hpp>
#include <mbootcore/runtime/IFirmwareService.hpp>
#include <mbootcore/runtime/IWorkflowService.hpp>
#include <mbootcore/runtime/IPluginService.hpp>
#include <mbootcore/runtime/IDiagnosticsService.hpp>

namespace mbootcore {
namespace runtime {

class Services {
public:
    virtual ~Services() = default;

    virtual IDeviceService& devices() = 0;

    virtual IFirmwareService& firmware() = 0;

    virtual IWorkflowService& workflows() = 0;

    virtual IPluginService& plugins() = 0;

    virtual IDiagnosticsService& diagnostics() = 0;
};

} // namespace runtime
} // namespace mbootcore
