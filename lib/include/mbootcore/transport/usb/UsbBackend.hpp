#pragma once

#include <mbootcore/transport/usb/UsbDeviceInfo.hpp>
#include <mbootcore/transport/usb/UsbEndpoint.hpp>
#include <mbootcore/transport/usb/UsbTransfer.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <vector>
#include <memory>
#include <functional>

namespace mbootcore {
namespace transport {
namespace usb {

/**
 * @brief Abstract interface for USB device backends.
 *
 * This interface is intentionally NOT prefixed with `I` for historical
 * compatibility; the naming convention will be aligned in a future major release.
 *
 * Implemented by WinUsbBackend (Windows) and LibUsbBackend (Linux/macOS).
 * USB backends are optional — makeUsbBackend() may return nullptr if no
 * USB library is available at build time.
 *
 * Implementations must be thread-safe unless documented otherwise.
 */
class UsbBackend {
public:
    virtual ~UsbBackend() = default;

    /// @brief Returns true if this backend type is available on the current platform.
    virtual bool isAvailable() const noexcept = 0;

    /**
     * @brief Opens a USB device by vendor/product ID.
     *
     * @param vendorId         USB vendor ID (e.g. 0x05C6 for Qualcomm).
     * @param productId        USB product ID.
     * @param interfaceNumber  Interface to claim (default 0).
     * @return Result<void> Ok on success, error code on failure.
     */
    virtual Result<void> open(uint16_t vendorId, uint16_t productId,
                              int interfaceNumber = 0) = 0;

    /// @brief Closes the USB device. Safe to call if not open.
    virtual void close() noexcept = 0;

    /// @brief Returns true if the device is currently open.
    virtual bool isOpen() const noexcept = 0;

    /**
     * @brief Performs a bulk IN transfer from the device.
     *
     * @param endpoint  Endpoint address (e.g. 0x81).
     * @param buffer    Destination buffer.
     * @param size      Maximum number of bytes to read.
     * @param timeout   Maximum time to wait for the transfer.
     * @return Result<size_t> Number of bytes read, or error code.
     */
    virtual Result<size_t> bulkRead(uint8_t endpoint, uint8_t* buffer, size_t size,
                                    std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Performs a bulk OUT transfer to the device.
     *
     * @param endpoint  Endpoint address (e.g. 0x01).
     * @param data      Buffer to send.
     * @param size      Number of bytes to send.
     * @param timeout   Maximum time to wait for the transfer.
     * @return Result<size_t> Number of bytes written, or error code.
     */
    virtual Result<size_t> bulkWrite(uint8_t endpoint, const uint8_t* data, size_t size,
                                     std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Performs a control READ transfer (host-to-device).
     *
     * @param requestType bmRequestType byte.
     * @param request     bRequest byte.
     * @param value       wValue field.
     * @param index       wIndex field.
     * @param buffer      Destination buffer.
     * @param size        Maximum number of bytes to read.
     * @param timeout     Maximum time to wait.
     * @return Result<size_t> Number of bytes read, or error code.
     */
    virtual Result<size_t> controlRead(uint8_t requestType, uint8_t request,
                                       uint16_t value, uint16_t index,
                                       uint8_t* buffer, size_t size,
                                       std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Performs a control WRITE transfer (device-to-host).
     *
     * @param requestType bmRequestType byte.
     * @param request     bRequest byte.
     * @param value       wValue field.
     * @param index       wIndex field.
     * @param data        Buffer to send.
     * @param size        Number of bytes to send.
     * @param timeout     Maximum time to wait.
     * @return Result<size_t> Number of bytes written, or error code.
     */
    virtual Result<size_t> controlWrite(uint8_t requestType, uint8_t request,
                                        uint16_t value, uint16_t index,
                                        const uint8_t* data, size_t size,
                                        std::chrono::milliseconds timeout) = 0;

    /// @brief Claims a USB interface for exclusive use.
    virtual Result<void> claimInterface(int interfaceNumber) = 0;

    /// @brief Releases a previously claimed interface.
    virtual Result<void> releaseInterface(int interfaceNumber) = 0;

    /// @brief Performs a USB device reset.
    virtual Result<void> resetDevice() = 0;

    /// @brief Resets a specific pipe (endpoint) on the device.
    virtual Result<void> resetPipe(uint8_t endpoint) = 0;

    /// @brief Aborts any pending transfer on the specified endpoint.
    virtual Result<void> abortPipe(uint8_t endpoint) = 0;

    /// @brief Returns a snapshot of the device descriptor information.
    virtual Result<UsbDeviceInfo> deviceInfo() const = 0;

    /// @brief Returns a human-readable name identifying the backend implementation.
    virtual std::string backendName() const noexcept = 0;

    /// @brief Sets or clears the logger used for diagnostic output.
    virtual void setLogger(ILogger* logger) noexcept = 0;
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
