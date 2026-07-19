#pragma once

#include <cstdint>
#include <string>

namespace mbootcore {
namespace transport {
namespace usb {

/// @brief Data flow direction for a USB endpoint.
enum class EndpointDirection : uint8_t {
    Out = 0,
    In = 1
};

/// @brief USB transfer type.
enum class TransferType : uint8_t {
    Control = 0,
    Isochronous,
    Bulk,
    Interrupt
};

/// @brief USB endpoint descriptor.
struct UsbEndpoint {
    uint8_t endpointAddress{0};
    EndpointDirection direction{EndpointDirection::In};
    TransferType transferType{TransferType::Bulk};
    uint16_t maxPacketSize{512};
    uint8_t interval{0};
    std::string description;

    bool isIn() const noexcept { return direction == EndpointDirection::In; }
    bool isOut() const noexcept { return direction == EndpointDirection::Out; }
    uint8_t endpointNumber() const noexcept { return endpointAddress & 0x0F; }
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
