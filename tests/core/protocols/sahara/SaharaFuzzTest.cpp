#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

#include <random>
#include <climits>

namespace {

std::mt19937 m_rng{42};
mbootcore::SaharaPacketParser m_parser;
mbootcore::SaharaPacketSerializer m_serializer;

mbootcore::ByteBuffer randomBuffer(size_t size) {
    mbootcore::ByteBuffer buf(size);
    for (auto& b : buf) b = static_cast<uint8_t>(m_rng() & 0xFF);
    return buf;
}

void fuzzParse(const mbootcore::ByteBuffer& data) {
    auto result = m_parser.parse(data);
    (void)result;
}

// fuzzSerialize intentionally unused; kept for reference

} // anonymous namespace

TEST_CASE("SaharaFuzzTest", "[sahara]") {
    SECTION("testEmptyBuffer") {
        fuzzParse({});
    }

    SECTION("testShortBuffer") {
        for (size_t len = 1; len < 8; ++len) {
            fuzzParse(randomBuffer(len));
        }
    }

    SECTION("testMinimalHeader") {
        mbootcore::ByteBuffer buf(8, 0);
        buf[0] = 0x01;
        buf[4] = 0x30; buf[5] = 0x00; buf[6] = 0x00; buf[7] = 0x00;
        fuzzParse(buf);
    }

    SECTION("testUnknownCommand") {
        for (uint32_t cmd = 0x14; cmd <= 0xFF; ++cmd) {
            mbootcore::ByteBuffer buf(8, 0);
            buf[0] = static_cast<uint8_t>(cmd & 0xFF);
            buf[1] = static_cast<uint8_t>((cmd >> 8) & 0xFF);
            buf[2] = static_cast<uint8_t>((cmd >> 16) & 0xFF);
            buf[3] = static_cast<uint8_t>((cmd >> 24) & 0xFF);
            buf[4] = 0x08; buf[5] = 0x00; buf[6] = 0x00; buf[7] = 0x00;
            fuzzParse(buf);
        }
    }

    SECTION("testMalformedLength") {
        mbootcore::ByteBuffer buf(8, 0);
        buf[4] = 0xE8; buf[5] = 0x03; buf[6] = 0x00; buf[7] = 0x00;
        fuzzParse(buf);
    }

    SECTION("testZeroLength") {
        mbootcore::ByteBuffer buf(8, 0);
        buf[0] = 0x05;
        fuzzParse(buf);
    }

    SECTION("testLengthSmallerThanHeader") {
        mbootcore::ByteBuffer buf(8, 0);
        buf[0] = 0x05;
        buf[4] = 0x04; buf[5] = 0x00; buf[6] = 0x00; buf[7] = 0x00;
        fuzzParse(buf);
    }

    SECTION("testCommandZero") {
        mbootcore::ByteBuffer buf(8, 0);
        buf[4] = 0x08;
        fuzzParse(buf);
    }

    SECTION("testRandomBuffers") {
        for (size_t len : {8, 16, 32, 64, 128, 256, 512}) {
            for (int i = 0; i < 50; ++i) {
                fuzzParse(randomBuffer(len));
            }
        }
    }

    SECTION("testLargeBuffer") {
        auto buf = randomBuffer(65536);
        fuzzParse(buf);
    }

    SECTION("testCorruptedHeader") {
        for (int i = 0; i < 8; ++i) {
            for (int val = 0; val < 256; val += 16) {
                mbootcore::ByteBuffer buf(8, 0);
                buf[i] = static_cast<uint8_t>(val);
                fuzzParse(buf);
            }
        }
    }

    SECTION("testReadChipIdV2vsV3") {
        mbootcore::ByteBuffer v2buf = {
            0x0A, 0x00, 0x00, 0x00,
            0x10, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x10, 0x00,
            0x01, 0x00, 0x00, 0x00,
        };
        auto v2result = m_parser.parse(v2buf);
        REQUIRE(v2result.isOk());
        auto& v2pkt = static_cast<const mbootcore::ReadChipIdPacket&>(*v2result.value());
        REQUIRE(!v2pkt.isV3());
        REQUIRE(v2pkt.chipIdLo() == uint32_t(0x00100000));
        REQUIRE(v2pkt.chipIdHi() == uint32_t(0x00000001));

        mbootcore::ByteBuffer v3buf(40, 0);
        v3buf[0] = 0x0A; v3buf[1] = 0x00; v3buf[2] = 0x00; v3buf[3] = 0x00;
        v3buf[4] = 0x28; v3buf[5] = 0x00; v3buf[6] = 0x00; v3buf[7] = 0x00;
        v3buf[8] = 0x78; v3buf[9] = 0x56; v3buf[10] = 0x34; v3buf[11] = 0x12;
        v3buf[12] = 0x01; v3buf[13] = 0x00; v3buf[14] = 0x00; v3buf[15] = 0x00;
        v3buf[16] = 0xDD; v3buf[17] = 0xCC; v3buf[18] = 0xBB; v3buf[19] = 0xAA;
        v3buf[20] = 0x01; v3buf[21] = 0x00; v3buf[22] = 0x07; v3buf[23] = 0x00;
        v3buf[24] = 0x01; v3buf[25] = 0x00; v3buf[26] = 0x00; v3buf[27] = 0x00;
        v3buf[28] = 0x05; v3buf[29] = 0x00; v3buf[30] = 0x00; v3buf[31] = 0x00;
        auto v3result = m_parser.parse(v3buf);
        REQUIRE(v3result.isOk());
        auto& v3pkt = static_cast<const mbootcore::ReadChipIdPacket&>(*v3result.value());
        REQUIRE(v3pkt.isV3());
        REQUIRE(v3pkt.chipIdLo() == uint32_t(0x12345678));
        REQUIRE(v3pkt.serialNum() == uint32_t(0xAABBCCDD));
        REQUIRE(v3pkt.msmId() == uint32_t(0x00070001));
    }

    SECTION("testSerializeAfterFuzzParse") {
        auto buf = randomBuffer(64);
        auto parseResult = m_parser.parse(buf);
        if (parseResult.isOk()) {
            auto serResult = m_serializer.serialize(*parseResult.value());
            (void)serResult;
        }
    }

    SECTION("testInvalidVersionsDoNotCrash") {
        mbootcore::HelloPacket hello(0, 0, 4096, 0);
        auto ser = m_serializer.serialize(hello);
        REQUIRE(ser.isOk());
        auto parsed = m_parser.parse(ser.value());
        REQUIRE(parsed.isOk());
    }
}
