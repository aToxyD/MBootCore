#include <catch2/catch_test_macros.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeBuilder.hpp>
#include <mbootcore/transport/usb/makeUsbBackend.hpp>
#include <algorithm>
#include <string>

using namespace mbootcore;
using namespace mbootcore::transport::usb;
using namespace mbootcore::runtime;

TEST_CASE("UsbCapabilityConsistency", "[usb][capabilities]") {
    SECTION("makeUsbBackend_matchesExpectation") {
        auto backend = makeUsbBackend();
#ifdef MBOOTCORE_TEST_USB_ENABLED
        REQUIRE(backend);
        REQUIRE(!backend->backendName().empty());
#else
        REQUIRE_FALSE(backend);
#endif
    }

    SECTION("runtimeCapabilities_containUsbWhenExpected") {
        auto rt = RuntimeBuilder().build();
        auto caps = rt.capabilities();

        bool hasUsb = std::find(caps.begin(), caps.end(), "usb-backend") != caps.end();
        bool hasLibusb = std::find(caps.begin(), caps.end(), "libusb") != caps.end();
        bool hasWinusb = std::find(caps.begin(), caps.end(), "winusb") != caps.end();

#ifdef MBOOTCORE_TEST_USB_ENABLED
        REQUIRE(hasUsb);
        bool hasVendorCap = hasLibusb || hasWinusb;
        REQUIRE(hasVendorCap);
#else
        REQUIRE_FALSE(hasUsb);
        REQUIRE_FALSE(hasLibusb);
        REQUIRE_FALSE(hasWinusb);
#endif
    }

    SECTION("usbVendorSpecificCapability_matchesExpectation") {
        auto rt = RuntimeBuilder().build();
        auto caps = rt.capabilities();

        bool hasLibusb = std::find(caps.begin(), caps.end(), "libusb") != caps.end();
        bool hasWinusb = std::find(caps.begin(), caps.end(), "winusb") != caps.end();

#if defined(MBOOTCORE_TEST_USB_ENABLED) && defined(MBOOTCORE_TEST_USB_IS_LIBUSB)
        REQUIRE(hasLibusb);
        REQUIRE_FALSE(hasWinusb);
#elif defined(MBOOTCORE_TEST_USB_ENABLED) && defined(MBOOTCORE_TEST_USB_IS_WINUSB)
        REQUIRE(hasWinusb);
        REQUIRE_FALSE(hasLibusb);
#elif !defined(MBOOTCORE_TEST_USB_ENABLED)
        REQUIRE_FALSE(hasLibusb);
        REQUIRE_FALSE(hasWinusb);
#endif
    }

    SECTION("isWinUsbAvailable_matchesPlatform") {
#ifdef _WIN32
        isWinUsbAvailable();
#else
        REQUIRE_FALSE(isWinUsbAvailable());
#endif
    }

    SECTION("isLibUsbAvailable_matchesPlatform") {
#ifndef _WIN32
        isLibUsbAvailable();
#else
        REQUIRE_FALSE(isLibUsbAvailable());
#endif
    }
}
