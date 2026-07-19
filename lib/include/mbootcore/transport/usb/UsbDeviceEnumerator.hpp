#pragma once

#include <mbootcore/transport/usb/UsbDeviceInfo.hpp>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace transport {
namespace usb {

/// @brief Filter criteria for USB device enumeration.
struct EnumerateFilter {
    uint16_t vendorId{0};
    uint16_t productId{0};
    uint8_t deviceClass{0};
    bool bootModeOnly{false};
};

/// @brief USB device discovery and enumeration utility.
class UsbDeviceEnumerator {
public:
    /// @brief Enumerates all USB devices matching the given filter.
    static std::vector<UsbDeviceInfo> enumerate(const EnumerateFilter& filter = {});

    /// @brief Finds all devices with the given vendor ID.
    static std::vector<UsbDeviceInfo> findByVendor(uint16_t vendorId);

    /// @brief Finds all devices matching the given vendor and product IDs.
    static std::vector<UsbDeviceInfo> findByProduct(uint16_t vendorId, uint16_t productId);

    /// @brief Returns the first device matching the given vendor and product ID.
    static UsbDeviceInfo findFirst(uint16_t vendorId, uint16_t productId);

    /// @brief Returns the total number of USB devices on the system.
    static size_t deviceCount();

    /// @brief Enumerates devices in known boot-mode configurations (e.g. Qualcomm EDL).
    static std::vector<UsbDeviceInfo> findBootModeDevices();
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
