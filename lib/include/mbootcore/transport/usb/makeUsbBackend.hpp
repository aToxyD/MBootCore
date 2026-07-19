#pragma once

#include <mbootcore/transport/usb/UsbBackend.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <memory>

namespace mbootcore {
namespace transport {
namespace usb {

/**
 * @brief Creates the best available USB backend for the current platform.
 *
 * Selection order (compile-time):
 *   1. WinUsbBackend  (Windows)
 *   2. LibUsbBackend  (Linux/macOS, requires libusb-1.0)
 *
 * USB support is optional — this function may return nullptr if no USB
 * library was available at build time.
 *
 * @param logger Optional logger for diagnostics (may be nullptr).
 * @return A unique_ptr to the backend, or nullptr if no USB backend is available.
 */
std::unique_ptr<UsbBackend> makeUsbBackend(ILogger* logger = nullptr);

/// @brief Returns true if the WinUSB backend is available on this system.
bool isWinUsbAvailable() noexcept;

/// @brief Returns true if the libusb backend is available on this system.
bool isLibUsbAvailable() noexcept;

} // namespace usb
} // namespace transport
} // namespace mbootcore
