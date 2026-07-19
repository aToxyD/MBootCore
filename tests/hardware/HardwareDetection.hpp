#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mbootcore::testing::hardware {

struct HardwareDeviceInfo
{
    std::string vendor;
    std::string model;
    std::string transport;
    std::string port;
    uint16_t usbVid{0};
    uint16_t usbPid{0};
};

class HardwareDetection
{
public:
    static bool isAnyDevicePresent() noexcept
    {
        return isQualcommDevicePresent()
            || isMediaTekDevicePresent()
            || isSamsungDevicePresent()
            || isUniSocDevicePresent()
            || isRockchipDevicePresent();
    }

    static bool isQualcommDevicePresent() noexcept
    {
        return false;
    }

    static bool isMediaTekDevicePresent() noexcept
    {
        return false;
    }

    static bool isSamsungDevicePresent() noexcept
    {
        return false;
    }

    static bool isUniSocDevicePresent() noexcept
    {
        return false;
    }

    static bool isRockchipDevicePresent() noexcept
    {
        return false;
    }

    static std::vector<HardwareDeviceInfo> enumerateDevices() noexcept
    {
        return {};
    }
};

} // namespace mbootcore::testing::hardware

#define MBOOT_REQUIRE_HARDWARE(condition, vendor)                              \
    do {                                                                       \
        if (!(condition)) {                                                    \
            SKIP(vendor " hardware not connected");                            \
        }                                                                      \
    } while (false)
