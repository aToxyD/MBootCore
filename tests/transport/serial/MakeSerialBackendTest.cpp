#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/serial/makeSerialBackend.hpp>

using namespace mbootcore::transport::serial;

TEST_CASE("MakeSerialBackendTest", "[transport]") {
    SECTION("makeSerialBackend_returnsNonNull") {
        auto backend = makeSerialBackend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->isAvailable());
    }

    SECTION("makeSerialBackend_backendName_matchesExpected") {
        auto backend = makeSerialBackend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->backendName() == MBOOTCORE_EXPECTED_SERIAL_BACKEND);
    }
}
