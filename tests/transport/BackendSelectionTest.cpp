#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/serial/makeSerialBackend.hpp>
#include <mbootcore/transport/network/makeTcpBackend.hpp>
#include <mbootcore/transport/network/makeUdpBackend.hpp>
#include <mbootcore/transport/usb/makeUsbBackend.hpp>

using namespace mbootcore::transport::serial;
using namespace mbootcore::transport::network;
using namespace mbootcore::transport::usb;

TEST_CASE("BackendSelectionTest", "[backend][selection]") {

    SECTION("serial_returnsNonNull") {
        auto b = makeSerialBackend();
        REQUIRE(b != nullptr);
    }

    SECTION("serial_isAvailable") {
        auto b = makeSerialBackend();
        REQUIRE(b->isAvailable());
    }

    SECTION("serial_backendName_matchesExpected") {
        auto b = makeSerialBackend();
        REQUIRE(b->backendName() == MBOOTCORE_EXPECTED_SERIAL_BACKEND);
    }

    SECTION("serial_acceptsLogger") {
        auto b = makeSerialBackend(nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(b->isAvailable());
    }

    SECTION("tcp_returnsNonNull") {
        auto b = makeTcpBackend();
        REQUIRE(b != nullptr);
    }

    SECTION("tcp_isAvailable") {
        auto b = makeTcpBackend();
        REQUIRE(b->isAvailable());
    }

    SECTION("tcp_backendName_matchesExpected") {
        auto b = makeTcpBackend();
        REQUIRE(b->backendName() == MBOOTCORE_EXPECTED_TCP_BACKEND);
    }

    SECTION("tcp_acceptsLogger") {
        auto b = makeTcpBackend(nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(b->isAvailable());
    }

    SECTION("udp_returnsNonNull") {
        auto b = makeUdpBackend();
        REQUIRE(b != nullptr);
    }

    SECTION("udp_isAvailable") {
        auto b = makeUdpBackend();
        REQUIRE(b->isAvailable());
    }

    SECTION("udp_backendName_matchesExpected") {
        auto b = makeUdpBackend();
        REQUIRE(b->backendName() == MBOOTCORE_EXPECTED_UDP_BACKEND);
    }

    SECTION("udp_acceptsLogger") {
        auto b = makeUdpBackend(nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(b->isAvailable());
    }

    SECTION("usb_mayReturnNull") {
        auto b = makeUsbBackend();
        if (b) {
            REQUIRE(!b->backendName().empty());
        }
    }

    SECTION("usb_acceptsLogger") {
        auto b = makeUsbBackend(nullptr);
        if (b) {
            REQUIRE(b->isAvailable());
        }
    }

    SECTION("usb_backendName_isNotEmpty") {
        auto b = makeUsbBackend();
        if (b) {
            REQUIRE(!b->backendName().empty());
        }
    }
}
