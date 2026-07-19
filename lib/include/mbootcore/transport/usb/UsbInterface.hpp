#pragma once

#include <mbootcore/transport/usb/UsbEndpoint.hpp>
#include <vector>
#include <string>

namespace mbootcore {
namespace transport {
namespace usb {

/// @brief USB interface descriptor with associated endpoints.
struct UsbInterface {
    uint8_t interfaceNumber{0};
    uint8_t alternateSetting{0};
    uint8_t interfaceClass{0};
    uint8_t interfaceSubClass{0};
    uint8_t interfaceProtocol{0};
    std::string description;
    std::vector<UsbEndpoint> endpoints;

    /// @brief Finds an endpoint matching the given direction and transfer type (non-const).
    UsbEndpoint* findEndpoint(EndpointDirection dir, TransferType type = TransferType::Bulk) {
        for (auto& ep : endpoints) {
            if (ep.direction == dir && ep.transferType == type) return &ep;
        }
        return nullptr;
    }

    /// @brief Finds an endpoint matching the given direction and transfer type (const).
    const UsbEndpoint* findEndpoint(EndpointDirection dir, TransferType type = TransferType::Bulk) const {
        for (const auto& ep : endpoints) {
            if (ep.direction == dir && ep.transferType == type) return &ep;
        }
        return nullptr;
    }
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
