#include <catch2/catch_test_macros.hpp>

#include "HardwareDetection.hpp"

#include <iostream>

using namespace mbootcore::testing::hardware;

TEST_CASE("QualcommHardwareTest", "[hardware]") {

    SECTION("test_deviceDetection")
    {
        MBOOT_REQUIRE_HARDWARE(HardwareDetection::isQualcommDevicePresent(),
                               "Qualcomm");

        const auto devices = HardwareDetection::enumerateDevices();
        REQUIRE(!devices.empty());

        bool foundQualcomm = false;
        for (const auto& dev : devices) {
            if (dev.vendor == "Qualcomm") {
                foundQualcomm = true;
                std::cout << "[Hardware] Qualcomm device: " << dev.model
                          << " via " << dev.transport << " " << dev.port
                          << std::endl;
            }
        }
        INFO("Qualcomm device not found in enumeration");
        REQUIRE(foundQualcomm);
    }

    SECTION("test_saharaHello")
    {
        MBOOT_REQUIRE_HARDWARE(HardwareDetection::isQualcommDevicePresent(),
                               "Qualcomm");
    }

    SECTION("test_saharaMemoryDebug")
    {
        MBOOT_REQUIRE_HARDWARE(HardwareDetection::isQualcommDevicePresent(),
                               "Qualcomm");
    }

    SECTION("test_firehoseHello")
    {
        MBOOT_REQUIRE_HARDWARE(HardwareDetection::isQualcommDevicePresent(),
                               "Qualcomm");
    }

    SECTION("test_firehoseGetStorageInfo")
    {
        MBOOT_REQUIRE_HARDWARE(HardwareDetection::isQualcommDevicePresent(),
                               "Qualcomm");
    }
}
