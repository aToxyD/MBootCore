#include <catch2/catch_test_macros.hpp>
#include "HardwareDetection.hpp"

using namespace mbootcore::testing::hardware;

TEST_CASE("MediaTekHardwareTest", "[hardware][scaffold]") {
    SECTION("test_deviceDetection") {
        auto devices = HardwareDetection::enumerateDevices();
        bool mtkFound = false;
        for (const auto& dev : devices) {
            if (dev.vendor == "MediaTek") {
                mtkFound = true;
                REQUIRE(!dev.model.empty());
                REQUIRE(dev.usbVid != 0);
            }
        }
        if (!mtkFound) {
            MBOOT_REQUIRE_HARDWARE(false, "MediaTek");
        }
    }

    SECTION("test_bromHandshake") {
        MBOOT_REQUIRE_HARDWARE(
            HardwareDetection::isMediaTekDevicePresent(), "MediaTek");
    }

    SECTION("test_getVersion") {
        MBOOT_REQUIRE_HARDWARE(
            HardwareDetection::isMediaTekDevicePresent(), "MediaTek");
    }

    SECTION("test_getHwCode") {
        MBOOT_REQUIRE_HARDWARE(
            HardwareDetection::isMediaTekDevicePresent(), "MediaTek");
    }
}
