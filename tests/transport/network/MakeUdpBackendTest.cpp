#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/network/makeUdpBackend.hpp>

using namespace mbootcore::transport::network;

TEST_CASE("MakeUdpBackendTest", "[network]") {
    SECTION("makeUdpBackend_returnsNonNull") {
        auto backend = makeUdpBackend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->isAvailable());
    }

    SECTION("makeUdpBackend_backendName_matchesExpected") {
        auto backend = makeUdpBackend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->backendName() == MBOOTCORE_EXPECTED_UDP_BACKEND);
    }
}
