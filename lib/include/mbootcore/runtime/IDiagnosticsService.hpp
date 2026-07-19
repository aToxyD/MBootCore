#pragma once

#include <mbootcore/runtime/RuntimeStatistics.hpp>
#include <mbootcore/runtime/RuntimeHardware.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/transport/usb/UsbDeviceInfo.hpp>

#include <string>
#include <vector>

namespace mbootcore {
namespace runtime {

class IDiagnosticsService {
public:
    virtual ~IDiagnosticsService() = default;

    virtual RuntimeStatistics statistics() const = 0;

    virtual RuntimeHealth health() const = 0;

    virtual HardwareDiagnosticReport hardwareReport() const = 0;

    virtual std::vector<HardwareDeviceEntry> connectedHardware() const = 0;

    virtual HardwareTransportStats transportStatistics(
        transport::TransportType type) const = 0;

    virtual std::vector<transport::usb::UsbDeviceInfo> usbDevices() const = 0;

    virtual std::vector<std::string> serialPorts() const = 0;
};

} // namespace runtime
} // namespace mbootcore
