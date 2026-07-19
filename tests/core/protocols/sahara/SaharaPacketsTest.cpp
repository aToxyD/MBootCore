#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

TEST_CASE("SaharaPacketsTest", "[sahara]") {
    SECTION("testHelloPacket") {
        mbootcore::HelloPacket pkt(2, 2, 4096, 0);
        REQUIRE(pkt.command() == uint32_t(0x01));
        REQUIRE(pkt.length() == uint32_t(48));
        REQUIRE(pkt.version() == uint32_t(2));
        REQUIRE(pkt.versionSupported() == uint32_t(2));
        REQUIRE(pkt.cmdPacketLength() == uint32_t(4096));
        REQUIRE(pkt.mode() == uint32_t(0));

        auto clone = pkt.clone();
        REQUIRE(clone->command() == uint32_t(0x01));

        auto str = pkt.toString();
        REQUIRE(!str.empty());
    }

    SECTION("testHelloResponsePacket") {
        mbootcore::HelloResponsePacket pkt(2, 2, 0, 0);
        REQUIRE(pkt.command() == uint32_t(0x02));
        REQUIRE(pkt.status() == uint32_t(0));
    }

    SECTION("testReadDataPacket") {
        mbootcore::ReadDataPacket pkt(0, 0x1000, 64);
        REQUIRE(pkt.command() == uint32_t(0x03));
        REQUIRE(pkt.imageId() == uint32_t(0));
        REQUIRE(pkt.dataOffset() == uint32_t(0x1000));
        REQUIRE(pkt.dataLength() == uint32_t(64));
    }

    SECTION("testEndImageTransferPacket") {
        mbootcore::EndImageTransferPacket pkt(0, 0);
        REQUIRE(pkt.command() == uint32_t(0x04));
        REQUIRE(pkt.status() == uint32_t(0));
    }

    SECTION("testDonePacket") {
        mbootcore::DonePacket pkt;
        REQUIRE(pkt.command() == uint32_t(0x05));
    }

    SECTION("testDoneResponsePacket") {
        mbootcore::DoneResponsePacket pkt(1);
        REQUIRE(pkt.command() == uint32_t(0x06));
        REQUIRE(pkt.imageTxStatus() == uint32_t(1));
    }

    SECTION("testResetPacket") {
        mbootcore::ResetPacket pkt;
        REQUIRE(pkt.command() == uint32_t(0x07));
    }
}
