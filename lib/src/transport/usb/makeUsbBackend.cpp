#include <mbootcore/transport/usb/makeUsbBackend.hpp>

#ifdef _WIN32
#include "transport/usb/WinUsbBackend.hpp"
#elif defined(MBOOTCORE_HAVE_LIBUSB)
#include "transport/usb/LibUsbBackend.hpp"
#endif

namespace mbootcore {
namespace transport {
namespace usb {

std::unique_ptr<UsbBackend> makeUsbBackend(ILogger* logger) {
#ifdef _WIN32
    return std::make_unique<WinUsbBackend>(logger);
#elif defined(MBOOTCORE_HAVE_LIBUSB)
    return std::make_unique<LibUsbBackend>(logger);
#else
    (void)logger;
    return nullptr;
#endif
}

bool isWinUsbAvailable() noexcept {
#ifdef _WIN32
    return WinUsbBackend::isWinUsbAvailable();
#else
    return false;
#endif
}

bool isLibUsbAvailable() noexcept {
#if defined(_WIN32) || !defined(MBOOTCORE_HAVE_LIBUSB)
    return false;
#else
    return LibUsbBackend::isLibUsbAvailable();
#endif
}

} // namespace usb
} // namespace transport
} // namespace mbootcore
