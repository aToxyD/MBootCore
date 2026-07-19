#include <catch2/catch_test_macros.hpp>
#include <mbootcore/network/NetworkAddress.hpp>
#include <unordered_map>
#include <sstream>

using namespace mbootcore;
using namespace mbootcore::network;

TEST_CASE("NetworkAddressTest", "[network]") {
    SECTION("default_constructed_isIPv4_any") {
        NetworkAddress addr;
        REQUIRE(addr.family() == AddressFamily::IPv4);
        REQUIRE(addr.port() == 0);
        REQUIRE(addr.toString() == "0.0.0.0");
    }

    SECTION("fromString_ipv4_parses") {
        auto res = NetworkAddress::fromString("192.168.1.1", 8080);
        REQUIRE(res.isOk());
        auto addr = res.value();
        REQUIRE(addr.family() == AddressFamily::IPv4);
        REQUIRE(addr.port() == 8080);
        REQUIRE(addr.toString() == "192.168.1.1:8080");
    }

    SECTION("fromString_ipv4_noPort") {
        auto res = NetworkAddress::fromString("10.0.0.1");
        REQUIRE(res.isOk());
        REQUIRE(res.value().toString() == "10.0.0.1");
    }

    SECTION("fromString_ipv6_parses") {
        auto res = NetworkAddress::fromString("::1", 443);
        REQUIRE(res.isOk());
        REQUIRE(res.value().family() == AddressFamily::IPv6);
        REQUIRE(res.value().port() == 443);
    }

    SECTION("fromString_empty_returnsError") {
        auto res = NetworkAddress::fromString("", 80);
        REQUIRE(res.isError());
    }

    SECTION("fromString_hostname") {
        auto res = NetworkAddress::fromString("localhost", 22);
        REQUIRE(res.isOk());
        REQUIRE(res.value().family() == AddressFamily::Hostname);
        REQUIRE(res.value().port() == 22);
        REQUIRE(res.value().toString() == "localhost:22");
    }

    SECTION("loopback_ipv4") {
        auto addr = NetworkAddress::loopback(AddressFamily::IPv4);
        REQUIRE(addr.toString() == "127.0.0.1");
    }

    SECTION("loopback_ipv6") {
        auto addr = NetworkAddress::loopback(AddressFamily::IPv6);
        REQUIRE(addr.toString() == "::1");
    }

    SECTION("loopback_default_is_ipv4") {
        auto addr = NetworkAddress::loopback();
        REQUIRE(addr.family() == AddressFamily::IPv4);
    }

    SECTION("any_ipv4") {
        auto addr = NetworkAddress::any(AddressFamily::IPv4);
        REQUIRE(addr.toString() == "0.0.0.0");
    }

    SECTION("any_ipv6") {
        auto addr = NetworkAddress::any(AddressFamily::IPv6);
        REQUIRE(addr.toString() == "::");
    }

    SECTION("equality_same") {
        auto a = NetworkAddress::fromString("10.0.0.1", 80);
        auto b = NetworkAddress::fromString("10.0.0.1", 80);
        REQUIRE((a.isOk() && b.isOk()));
        REQUIRE(a.value() == b.value());
        REQUIRE(!(a.value() != b.value()));
    }

    SECTION("equality_differentPort") {
        auto a = NetworkAddress::fromString("10.0.0.1", 80);
        auto b = NetworkAddress::fromString("10.0.0.1", 443);
        REQUIRE((a.isOk() && b.isOk()));
        REQUIRE(a.value() != b.value());
    }

    SECTION("equality_differentAddress") {
        auto a = NetworkAddress::fromString("10.0.0.1", 80);
        auto b = NetworkAddress::fromString("10.0.0.2", 80);
        REQUIRE((a.isOk() && b.isOk()));
        REQUIRE(a.value() != b.value());
    }

    SECTION("ordering") {
        auto a = NetworkAddress::fromString("10.0.0.1", 80);
        auto b = NetworkAddress::fromString("10.0.0.2", 80);
        REQUIRE((a.isOk() && b.isOk()));
        REQUIRE(a.value() < b.value());
        REQUIRE(!(b.value() < a.value()));
    }

    SECTION("hash_different") {
        auto a = NetworkAddress::fromString("10.0.0.1", 80);
        auto b = NetworkAddress::fromString("10.0.0.2", 80);
        REQUIRE((a.isOk() && b.isOk()));
        REQUIRE(a.value().hash() != b.value().hash());
    }

    SECTION("hash_same") {
        auto a = NetworkAddress::fromString("10.0.0.1", 80);
        auto b = NetworkAddress::fromString("10.0.0.1", 80);
        REQUIRE((a.isOk() && b.isOk()));
        REQUIRE(a.value().hash() == b.value().hash());
    }

    SECTION("hash_std_specialization") {
        std::unordered_map<NetworkAddress, int> map;
        auto addr = NetworkAddress::loopback();
        map[addr] = 42;
        REQUIRE(map[addr] == 42);
    }

    SECTION("copy_constructor") {
        auto res = NetworkAddress::fromString("10.0.0.1", 8080);
        REQUIRE(res.isOk());
        NetworkAddress copy(res.value());
        REQUIRE(copy.toString() == res.value().toString());
    }

    SECTION("copy_assignment") {
        auto res = NetworkAddress::fromString("10.0.0.1", 8080);
        REQUIRE(res.isOk());
        NetworkAddress copy;
        copy = res.value();
        REQUIRE(copy.toString() == res.value().toString());
    }

    SECTION("move_constructor") {
        auto res = NetworkAddress::fromString("10.0.0.1", 8080);
        REQUIRE(res.isOk());
        NetworkAddress moved(std::move(res.value()));
        REQUIRE(moved.toString() == "10.0.0.1:8080");
    }

    SECTION("stream_operator") {
        auto addr = NetworkAddress::loopback();
        std::ostringstream os;
        os << addr;
        REQUIRE(os.str() == "127.0.0.1");
    }
}
