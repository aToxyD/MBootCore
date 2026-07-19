#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/usb/makeUsbBackend.hpp>

using namespace mbootcore;
using namespace mbootcore::transport::usb;

TEST_CASE("MakeUsbBackendTest", "[usb]") {
    SECTION("makeUsbBackend_returnsBackendOrNull") {
        auto backend = makeUsbBackend();
        if (backend) {
            REQUIRE(!backend->backendName().empty());
        }
    }

    SECTION("isWinUsbAvailable_matchesPlatform") {
#ifdef _WIN32
        isWinUsbAvailable();
#else
        REQUIRE(!isWinUsbAvailable());
#endif
    }

    SECTION("isLibUsbAvailable_matchesPlatform") {
#ifndef _WIN32
        isLibUsbAvailable();
#else
        REQUIRE(!isLibUsbAvailable());
#endif
    }
}
