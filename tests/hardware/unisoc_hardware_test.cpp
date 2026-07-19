#include <catch2/catch_test_macros.hpp>
#include "HardwareDetection.hpp"

using namespace mbootcore::testing::hardware;

TEST_CASE("UNISOCHardwareTest", "[hardware][scaffold]") {
    SECTION("test_deviceDetection") {
        auto devices = HardwareDetection::enumerateDevices();
        bool unisocFound = false;
        for (const auto& dev : devices) {
            if (dev.vendor == "UNISOC") {
                unisocFound = true;
                REQUIRE(!dev.model.empty());
                REQUIRE(dev.usbVid != 0);
            }
        }
        if (!unisocFound) {
            MBOOT_REQUIRE_HARDWARE(false, "UNISOC");
        }
    }

    SECTION("test_bromHandshake") {
        MBOOT_REQUIRE_HARDWARE(
            HardwareDetection::isUniSocDevicePresent(), "UNISOC");
    }

    SECTION("test_getVersion") {
        MBOOT_REQUIRE_HARDWARE(
            HardwareDetection::isUniSocDevicePresent(), "UNISOC");
    }

    SECTION("test_getChipInfo") {
        MBOOT_REQUIRE_HARDWARE(
            HardwareDetection::isUniSocDevicePresent(), "UNISOC");
    }
}
