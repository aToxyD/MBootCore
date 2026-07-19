#pragma once

#ifdef _WIN32
#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace platform {

struct SerialPortInfo {
    std::string portName;
    std::string description;
    std::string manufacturer;
    std::string serialNumber;
    std::string hardwareId;
    uint16_t vid{0};
    uint16_t pid{0};
    std::string friendlyName;
};

std::vector<SerialPortInfo> enumerateSerialPorts();

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
