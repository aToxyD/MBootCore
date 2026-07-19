#pragma once

#include <mbootcore/transport/TransportTypes.hpp>
#include <vector>
#include <string>

namespace mbootcore {
namespace transport {

/// @brief Describes a serial port detected on the system.
struct SerialPortEntry {
    std::string portName;
    std::string description;
    std::string manufacturer;
    std::string serialNumber;
    TransportType type{TransportType::Serial};
};

/// @brief Enumerates serial ports available on the system.
class SerialEnumerator {
public:
    /// @brief Returns a list of all detected serial ports.
    static std::vector<SerialPortEntry> enumerate();

    /// @brief Finds serial ports whose description contains the given string.
    static std::vector<SerialPortEntry> findByDescription(const std::string& desc);

    /// @brief Returns the first detected serial port.
    static SerialPortEntry findFirst();

    /// @brief Returns the total number of serial ports detected.
    static size_t portCount();
};

} // namespace transport
} // namespace mbootcore
