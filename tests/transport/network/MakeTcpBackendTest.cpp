#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/network/makeTcpBackend.hpp>

using namespace mbootcore::transport::network;

TEST_CASE("MakeTcpBackendTest", "[network]") {
    SECTION("makeTcpBackend_returnsNonNull") {
        auto backend = makeTcpBackend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->isAvailable());
    }

    SECTION("makeTcpBackend_backendName_matchesExpected") {
        auto backend = makeTcpBackend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->backendName() == MBOOTCORE_EXPECTED_TCP_BACKEND);
    }
}
