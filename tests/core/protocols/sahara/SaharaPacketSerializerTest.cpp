#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

TEST_CASE("SaharaPacketSerializerTest", "[sahara]") {
    SECTION("testHelloPacketSerialize") {
        mbootcore::SaharaPacketSerializer serializer;
        mbootcore::HelloPacket packet(2, 2, 1024, 0);

        auto result = serializer.serialize(packet);
        REQUIRE(result.isOk());

        auto& buf = result.value();
        REQUIRE(buf.size() == size_t(48));
        REQUIRE(buf[0] == 0x01);
        REQUIRE(buf[1] == 0x00);
        REQUIRE(buf[4] == 0x30);
    }

    SECTION("testReadDataPacketSerialize") {
        mbootcore::SaharaPacketSerializer serializer;
        mbootcore::ReadDataPacket packet(0, 0, 64);

        auto result = serializer.serialize(packet);
        REQUIRE(result.isOk());

        auto& buf = result.value();
        REQUIRE(buf.size() == size_t(20));
        REQUIRE(buf[0] == 0x03);
    }

    SECTION("testDonePacketSerialize") {
        mbootcore::SaharaPacketSerializer serializer;
        mbootcore::DonePacket packet;

        auto result = serializer.serialize(packet);
        REQUIRE(result.isOk());

        auto& buf = result.value();
        REQUIRE(buf.size() == size_t(8));
        REQUIRE(buf[0] == 0x05);
    }

    SECTION("testCanSerialize") {
        mbootcore::SaharaPacketSerializer serializer;
        mbootcore::HelloPacket hello(2, 2, 1024, 0);
        REQUIRE(serializer.canSerialize(hello));
    }
}
