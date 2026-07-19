#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"

TEST_CASE("SaharaPacketParserTest", "[sahara]") {
    SECTION("testParseHelloPacket") {
        mbootcore::SaharaPacketParser parser;
        mbootcore::ByteBuffer data = {
            0x01, 0x00, 0x00, 0x00,
            0x30, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x00, 0x04, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        data.resize(48, 0);

        auto result = parser.parse(data);
        REQUIRE(result.isOk());
        REQUIRE(result.value()->command() == uint32_t(1));
    }

    SECTION("testIsComplete") {
        mbootcore::SaharaPacketParser parser;

        mbootcore::ByteBuffer incomplete = {0x01, 0x00};
        REQUIRE(!parser.isComplete(incomplete));

        mbootcore::ByteBuffer complete = {
            0x01, 0x00, 0x00, 0x00,
            0x08, 0x00, 0x00, 0x00
        };
        REQUIRE(parser.isComplete(complete));
    }

    SECTION("testParseInvalidData") {
        mbootcore::SaharaPacketParser parser;
        mbootcore::ByteBuffer empty;
        auto result = parser.parse(empty);
        REQUIRE(result.isError());
    }

    SECTION("truncated HelloPacket rejected") {
        mbootcore::SaharaPacketParser parser;
        // Declared length intentionally smaller than the protocol-defined
        // Hello packet size (48).
        mbootcore::ByteBuffer data = {
            0x01, 0x00, 0x00, 0x00,
            0x18, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x00, 0x04, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        auto r = parser.parse(data);
        REQUIRE(r.isError());
    }

    SECTION("truncated ReadDataPacket rejected") {
        mbootcore::SaharaPacketParser parser;
        // Declared length smaller than the protocol-defined
        // ReadData packet size (20).
        mbootcore::ByteBuffer data = {
            0x03, 0x00, 0x00, 0x00,
            0x0C, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00,
        };
        auto r = parser.parse(data);
        REQUIRE(r.isError());
    }

    SECTION("EndImageTransferPacket header-only rejected") {
        mbootcore::SaharaPacketParser parser;
        // Declared length covers only the header (8), missing both
        // required fields (imageId, status) at 16 bytes.
        mbootcore::ByteBuffer data = {
            0x04, 0x00, 0x00, 0x00,
            0x08, 0x00, 0x00, 0x00,
        };
        auto r = parser.parse(data);
        REQUIRE(r.isError());
    }

    SECTION("valid full-size packets parse successfully") {
        mbootcore::SaharaPacketParser parser;
        // DonePacket: len = 8
        mbootcore::ByteBuffer done = {
            0x05, 0x00, 0x00, 0x00,
            0x08, 0x00, 0x00, 0x00,
        };
        REQUIRE(parser.parse(done).isOk());
        // ReadDataPacket: len = 20
        mbootcore::ByteBuffer readData = {
            0x03, 0x00, 0x00, 0x00,
            0x14, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x04, 0x00, 0x00,
        };
        REQUIRE(parser.parse(readData).isOk());
        // HelloPacket: len = 48
        mbootcore::ByteBuffer hello = {
            0x01, 0x00, 0x00, 0x00,
            0x30, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00,
            0x00, 0x04, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
        hello.resize(48, 0);
        REQUIRE(parser.parse(hello).isOk());
    }

    SECTION("boundary — one byte below minimum rejected, minimum accepted") {
        mbootcore::SaharaPacketParser parser;
        mbootcore::ByteBuffer below = {
            0x01, 0x00, 0x00, 0x00,
            0x2F, 0x00, 0x00, 0x00,
        };
        below.resize(47, 0);
        REQUIRE(parser.parse(below).isError());

        mbootcore::ByteBuffer at = {
            0x01, 0x00, 0x00, 0x00,
            0x30, 0x00, 0x00, 0x00,
        };
        at.resize(48, 0);
        REQUIRE(parser.parse(at).isOk());
    }
}
