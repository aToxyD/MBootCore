#include "transport/usb/LibUsbBackend.hpp"
#include <mbootcore/domain/ILogger.hpp>

#include <libusb.h>
#include <sstream>
#include <iomanip>

namespace mbootcore {
namespace transport {
namespace usb {

namespace {

const char* libusbErrorString(int err) {
    switch (err) {
        case LIBUSB_SUCCESS: return "Success";
        case LIBUSB_ERROR_IO: return "I/O error";
        case LIBUSB_ERROR_INVALID_PARAM: return "Invalid parameter";
        case LIBUSB_ERROR_ACCESS: return "Access denied";
        case LIBUSB_ERROR_NO_DEVICE: return "No device";
        case LIBUSB_ERROR_NOT_FOUND: return "Device not found";
        case LIBUSB_ERROR_BUSY: return "Device busy";
        case LIBUSB_ERROR_TIMEOUT: return "Timeout";
        case LIBUSB_ERROR_OVERFLOW: return "Overflow";
        case LIBUSB_ERROR_PIPE: return "Pipe error";
        case LIBUSB_ERROR_INTERRUPTED: return "Interrupted";
        case LIBUSB_ERROR_NO_MEM: return "No memory";
        case LIBUSB_ERROR_NOT_SUPPORTED: return "Not supported";
        case LIBUSB_ERROR_OTHER: return "Other error";
        default: return "Unknown error";
    }
}

ErrorCode libusbToErrorCode(int err) {
    switch (err) {
        case LIBUSB_SUCCESS: return ErrorCode::Success;
        case LIBUSB_ERROR_TIMEOUT: return ErrorCode::TransportTimeout;
        case LIBUSB_ERROR_PIPE: return ErrorCode::TransportError;
        case LIBUSB_ERROR_NO_DEVICE: return ErrorCode::DeviceNotFound;
        case LIBUSB_ERROR_ACCESS: return ErrorCode::DeviceAccessDenied;
        case LIBUSB_ERROR_BUSY: return ErrorCode::TransportBusy;
        case LIBUSB_ERROR_OVERFLOW: return ErrorCode::TransportBufferFull;
        default: return ErrorCode::TransportError;
    }
}

} // anonymous namespace

LibUsbBackend::LibUsbBackend(ILogger* logger)
    : m_logger(logger) {}

LibUsbBackend::~LibUsbBackend() {
    close();
    if (m_ctx) {
        libusb_exit(m_ctx);
        m_ctx = nullptr;
    }
}

bool LibUsbBackend::isLibUsbAvailable() {
    libusb_context* testCtx = nullptr;
    int r = libusb_init(&testCtx);
    if (r == LIBUSB_SUCCESS) {
        libusb_exit(testCtx);
        return true;
    }
    return false;
}

bool LibUsbBackend::isAvailable() const noexcept {
    return true;
}

Result<void> LibUsbBackend::open(uint16_t vendorId, uint16_t productId, int interfaceNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_open) return ErrorCode::TransportAlreadyOpen;

    if (!m_ctx) {
        int r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS) {
            if (m_logger) m_logger->error("LibUsbBackend",
                std::string("libusb_init failed: ") + libusbErrorString(r));
            m_ctx = nullptr;
            return ErrorCode::TransportBackendUnavailable;
        }
    }

    m_handle = libusb_open_device_with_vid_pid(m_ctx, vendorId, productId);
    if (!m_handle) {
        if (m_logger) {
            std::ostringstream ss;
            ss << "Device not found: VID=0x" << std::hex << std::setw(4) << std::setfill('0')
               << vendorId << " PID=0x" << std::setw(4) << std::setfill('0') << productId;
            m_logger->error("LibUsbBackend", ss.str());
        }
        return ErrorCode::DeviceNotFound;
    }

    m_device = libusb_get_device(m_handle);

    if (interfaceNumber >= 0) {
        int r = libusb_claim_interface(m_handle, interfaceNumber);
        if (r != LIBUSB_SUCCESS) {
            if (m_logger) m_logger->error("LibUsbBackend",
                std::string("claim_interface failed: ") + libusbErrorString(r));
            libusb_close(m_handle);
            m_handle = nullptr;
            m_device = nullptr;
            return ErrorCode::TransportError;
        }
    }

    m_vendorId = vendorId;
    m_productId = productId;
    m_interfaceNumber = interfaceNumber;
    m_open = true;

    m_deviceInfo.vendorId = vendorId;
    m_deviceInfo.productId = productId;
    m_deviceInfo.isAvailable = true;

    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(m_device, &desc) == LIBUSB_SUCCESS) {
        m_deviceInfo.bcdDevice = desc.bcdDevice;
    }

    uint8_t busNumber = libusb_get_bus_number(m_device);
    uint8_t portNumber = libusb_get_port_number(m_device);
    std::ostringstream addr;
    addr << static_cast<int>(busNumber) << "-" << static_cast<int>(portNumber);
    m_deviceInfo.busAddress = addr.str();

    uint8_t speed = libusb_get_device_speed(m_device);
    switch (speed) {
        case LIBUSB_SPEED_LOW: m_deviceInfo.speed = UsbSpeed::Low; break;
        case LIBUSB_SPEED_FULL: m_deviceInfo.speed = UsbSpeed::Full; break;
        case LIBUSB_SPEED_HIGH: m_deviceInfo.speed = UsbSpeed::High; break;
        case LIBUSB_SPEED_SUPER: m_deviceInfo.speed = UsbSpeed::Super; break;
        case LIBUSB_SPEED_SUPER_PLUS: m_deviceInfo.speed = UsbSpeed::SuperPlus; break;
        default: m_deviceInfo.speed = UsbSpeed::Unknown; break;
    }

    if (m_logger) {
        std::ostringstream ss;
        ss << "Opened VID=0x" << std::hex << std::setw(4) << std::setfill('0')
           << vendorId << " PID=0x" << std::setw(4) << std::setfill('0') << productId
           << " iface=" << interfaceNumber;
        m_logger->info("LibUsbBackend", ss.str());
    }

    return {};
}

void LibUsbBackend::close() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return;

    if (m_handle) {
        if (m_interfaceNumber >= 0) {
            libusb_release_interface(m_handle, m_interfaceNumber);
        }
        libusb_close(m_handle);
        m_handle = nullptr;
        m_device = nullptr;
    }

    m_open = false;
    if (m_logger) m_logger->info("LibUsbBackend", "Closed");
}

bool LibUsbBackend::isOpen() const noexcept {
    return m_open;
}

Result<size_t> LibUsbBackend::bulkRead(uint8_t endpoint, uint8_t* buffer, size_t size,
                                        std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int transferred = 0;
    int r = libusb_bulk_transfer(m_handle, endpoint, buffer,
                                  static_cast<int>(size), &transferred,
                                  static_cast<unsigned int>(timeout.count()));

    if (r != LIBUSB_SUCCESS) {
        if (r == LIBUSB_ERROR_TIMEOUT) {
            return ErrorCode::TransportTimeout;
        }
        return libusbToErrorCode(r);
    }

    return static_cast<size_t>(transferred);
}

Result<size_t> LibUsbBackend::bulkWrite(uint8_t endpoint, const uint8_t* data, size_t size,
                                         std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int transferred = 0;
    int r = libusb_bulk_transfer(m_handle, endpoint,
                                  const_cast<uint8_t*>(data),
                                  static_cast<int>(size), &transferred,
                                  static_cast<unsigned int>(timeout.count()));

    if (r != LIBUSB_SUCCESS) {
        if (r == LIBUSB_ERROR_TIMEOUT) {
            return ErrorCode::TransportTimeout;
        }
        return libusbToErrorCode(r);
    }

    return static_cast<size_t>(transferred);
}

Result<size_t> LibUsbBackend::controlRead(uint8_t requestType, uint8_t request,
                                           uint16_t value, uint16_t index,
                                           uint8_t* buffer, size_t size,
                                           std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int r = libusb_control_transfer(m_handle,
                                     requestType | LIBUSB_ENDPOINT_IN,
                                     request, value, index,
                                     buffer, static_cast<uint16_t>(size),
                                     static_cast<unsigned int>(timeout.count()));

    if (r < 0) {
        if (r == LIBUSB_ERROR_TIMEOUT) {
            return ErrorCode::TransportTimeout;
        }
        return libusbToErrorCode(r);
    }

    return static_cast<size_t>(r);
}

Result<size_t> LibUsbBackend::controlWrite(uint8_t requestType, uint8_t request,
                                            uint16_t value, uint16_t index,
                                            const uint8_t* data, size_t size,
                                            std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int r = libusb_control_transfer(m_handle,
                                     requestType | LIBUSB_ENDPOINT_OUT,
                                     request, value, index,
                                     const_cast<uint8_t*>(data),
                                     static_cast<uint16_t>(size),
                                     static_cast<unsigned int>(timeout.count()));

    if (r < 0) {
        if (r == LIBUSB_ERROR_TIMEOUT) {
            return ErrorCode::TransportTimeout;
        }
        return libusbToErrorCode(r);
    }

    return static_cast<size_t>(r);
}

Result<void> LibUsbBackend::claimInterface(int interfaceNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int r = libusb_claim_interface(m_handle, interfaceNumber);
    if (r != LIBUSB_SUCCESS) {
        return libusbToErrorCode(r);
    }

    return {};
}

Result<void> LibUsbBackend::releaseInterface(int interfaceNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    libusb_release_interface(m_handle, interfaceNumber);
    return {};
}

Result<void> LibUsbBackend::resetDevice() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int r = libusb_reset_device(m_handle);
    if (r != LIBUSB_SUCCESS) {
        return libusbToErrorCode(r);
    }

    return {};
}

Result<void> LibUsbBackend::resetPipe(uint8_t endpoint) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    int r = libusb_clear_halt(m_handle, endpoint);
    if (r != LIBUSB_SUCCESS) {
        return libusbToErrorCode(r);
    }

    return {};
}

Result<void> LibUsbBackend::abortPipe(uint8_t endpoint) {
    return resetPipe(endpoint);
}

Result<UsbDeviceInfo> LibUsbBackend::deviceInfo() const {
    if (!m_open) return ErrorCode::TransportNotOpen;
    return m_deviceInfo;
}

} // namespace usb
} // namespace transport
} // namespace mbootcore
