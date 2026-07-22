#pragma once

#include <mbootcore/transport/usb/UsbBackend.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <string>
#include <mutex>
#include <memory>

namespace mbootcore {
namespace platform {
class DynamicLibrary;
}

namespace transport {
namespace usb {

class WinUsbBackend : public UsbBackend {
public:
    WinUsbBackend(ILogger* logger = nullptr);
    ~WinUsbBackend() override;

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
    std::string backendName() const noexcept override { return "WinUSB"; }

    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

    static bool isWinUsbAvailable();

private:
    struct WinUsbFunctions;
    std::unique_ptr<WinUsbFunctions> m_wusb;
    std::unique_ptr<platform::DynamicLibrary> m_lib;

    ILogger* m_logger{nullptr};
    // Cached platform capability determined during construction.
    // Does NOT indicate that a USB device is connected or opened.
    bool m_winUsbAvailable{false};
    std::mutex m_mutex;

    void* m_deviceHandle{nullptr};
    void* m_winUsbHandle{nullptr};

    UsbDeviceInfo m_deviceInfo;
    int m_interfaceNumber{0};
    bool m_open{false};

    std::string findDevicePath(uint16_t vendorId, uint16_t productId, int interfaceNumber);
    bool loadWinUsb();
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
