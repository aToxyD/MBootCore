#pragma once

#include <mbootcore/transport/usb/UsbInterface.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore {
namespace transport {
namespace usb {

/// @brief USB speed identifier.
enum class UsbSpeed : uint8_t {
    Unknown = 0,
    Low = 1,
    Full = 2,
    High = 3,
    Super = 4,
    SuperPlus = 5
};

/// @brief Full USB device descriptor information.
struct UsbDeviceInfo {
    uint16_t vendorId{0};
    uint16_t productId{0};
    uint16_t bcdDevice{0};
    std::string manufacturer;
    std::string product;
    std::string serialNumber;
    std::string devicePath;
    std::string busAddress;
    UsbSpeed speed{UsbSpeed::Unknown};
    bool isAvailable{false};
    std::vector<UsbInterface> interfaces;
    std::vector<uint8_t> rawDescriptor;

    /// @brief Finds a USB interface by its interface number (non-const).
    UsbInterface* findInterface(uint8_t number) {
        for (auto& iface : interfaces) {
            if (iface.interfaceNumber == number) return &iface;
        }
        return nullptr;
    }

    /// @brief Finds a USB interface by its interface number (const).
    const UsbInterface* findInterface(uint8_t number) const {
        for (const auto& iface : interfaces) {
            if (iface.interfaceNumber == number) return &iface;
        }
        return nullptr;
    }

    /// @brief Returns true if this device is a known Qualcomm/MediaTek boot-mode device.
    bool isBootMode() const noexcept {
        return (vendorId == 0x05C6 && productId == 0x9008) ||
               (vendorId == 0x05C6 && productId == 0x900E) ||
               (vendorId == 0x0E8D && productId == 0x0003) ||
               (vendorId == 0x18D1 && productId == 0xD00D);
    }

    /// @brief Returns a human-readable string describing the USB speed.
    std::string speedString() const {
        switch (speed) {
            case UsbSpeed::Low: return "Low (1.5 Mbps)";
            case UsbSpeed::Full: return "Full (12 Mbps)";
            case UsbSpeed::High: return "High (480 Mbps)";
            case UsbSpeed::Super: return "Super (5 Gbps)";
            case UsbSpeed::SuperPlus: return "Super+ (10 Gbps)";
            default: return "Unknown";
        }
    }
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
