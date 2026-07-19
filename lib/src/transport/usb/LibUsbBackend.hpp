#pragma once

#include <mbootcore/transport/usb/UsbBackend.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <string>
#include <mutex>
#include <memory>

struct libusb_context;
struct libusb_device_handle;
struct libusb_device;
struct libusb_device_descriptor;

namespace mbootcore {
namespace transport {
namespace usb {

class LibUsbBackend : public UsbBackend {
public:
    LibUsbBackend(ILogger* logger = nullptr);
    ~LibUsbBackend() override;

    bool isAvailable() const noexcept override;

    Result<void> open(uint16_t vendorId, uint16_t productId,
                      int interfaceNumber = 0) override;
    void close() noexcept override;
    bool isOpen() const noexcept override;

    Result<size_t> bulkRead(uint8_t endpoint, uint8_t* buffer, size_t size,
                            std::chrono::milliseconds timeout) override;
    Result<size_t> bulkWrite(uint8_t endpoint, const uint8_t* data, size_t size,
                             std::chrono::milliseconds timeout) override;

    Result<size_t> controlRead(uint8_t requestType, uint8_t request,
                                uint16_t value, uint16_t index,
                                uint8_t* buffer, size_t size,
                                std::chrono::milliseconds timeout) override;
    Result<size_t> controlWrite(uint8_t requestType, uint8_t request,
                                 uint16_t value, uint16_t index,
                                 const uint8_t* data, size_t size,
                                 std::chrono::milliseconds timeout) override;

    Result<void> claimInterface(int interfaceNumber) override;
    Result<void> releaseInterface(int interfaceNumber) override;

    Result<void> resetDevice() override;
    Result<void> resetPipe(uint8_t endpoint) override;
    Result<void> abortPipe(uint8_t endpoint) override;

    Result<UsbDeviceInfo> deviceInfo() const override;
    std::string backendName() const noexcept override { return "LibUsb"; }

    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

    static bool isLibUsbAvailable();

private:
    ILogger* m_logger{nullptr};
    std::mutex m_mutex;

    libusb_context* m_ctx{nullptr};
    libusb_device_handle* m_handle{nullptr};
    libusb_device* m_device{nullptr};

    UsbDeviceInfo m_deviceInfo;
    uint16_t m_vendorId{0};
    uint16_t m_productId{0};
    int m_interfaceNumber{0};
    bool m_open{false};
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
