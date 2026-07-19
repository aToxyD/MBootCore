#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"

TEST_CASE("SaharaGoldenVectorsTest", "[sahara]") {
    SECTION("testHelloRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::HelloPacket original(2, 2, 4096, 0);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::HelloPacket&>(*parRes.value());
        REQUIRE(pkt.version() == 2u);
        REQUIRE(pkt.versionSupported() == 2u);
        REQUIRE(pkt.cmdPacketLength() == 4096u);
        REQUIRE(pkt.mode() == 0u);
    }

    SECTION("testHelloResponseRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::HelloResponsePacket original(2, 2, 0, 0);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::HelloResponsePacket&>(*parRes.value());
        REQUIRE(pkt.version() == 2u);
        REQUIRE(pkt.status() == 0u);
        REQUIRE(pkt.mode() == 0u);
    }

    SECTION("testReadDataRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ReadDataPacket original(0, 0x1000, 64);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::ReadDataPacket&>(*parRes.value());
        REQUIRE(pkt.imageId() == 0u);
        REQUIRE(pkt.dataOffset() == 0x1000u);
        REQUIRE(pkt.dataLength() == 64u);
    }

    SECTION("testEndImageTransferRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::EndImageTransferPacket original(0, 0);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::EndImageTransferPacket&>(*parRes.value());
        REQUIRE(pkt.imageId() == 0u);
        REQUIRE(pkt.status() == 0u);
    }

    SECTION("testDoneRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::DonePacket original;
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        REQUIRE(parRes.value()->command() == 0x05u);
    }

    SECTION("testDoneResponseRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::DoneResponsePacket original(0);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::DoneResponsePacket&>(*parRes.value());
        REQUIRE(pkt.imageTxStatus() == 0u);
    }

    SECTION("testResetRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ResetPacket original;
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        REQUIRE(parRes.value()->command() == 0x07u);
    }

    SECTION("testResetResponseRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ResetResponsePacket original;
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        REQUIRE(parRes.value()->command() == 0x08u);
    }

    SECTION("testMemoryDebugRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::MemoryDebugPacket original(0x80000000, 256);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::MemoryDebugPacket&>(*parRes.value());
        REQUIRE(pkt.memTableAddr() == 0x80000000u);
        REQUIRE(pkt.memTableLen() == 256u);
    }

    SECTION("testReadChipIdV2RoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ReadChipIdPacket original(0x00100000, 0x00000001);
        REQUIRE(!original.isV3());
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::ReadChipIdPacket&>(*parRes.value());
        REQUIRE(pkt.chipIdLo() == 0x00100000u);
        REQUIRE(pkt.chipIdHi() == 0x00000001u);
        REQUIRE(!pkt.isV3());
    }

    SECTION("testReadChipIdV3RoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ReadChipIdPacket original(0x00100000, 0x00000001,
                                                 0x12345678, 0x00070001,
                                                 0x00000001, 0x00000005);
        REQUIRE(original.isV3());
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::ReadChipIdPacket&>(*parRes.value());
        REQUIRE(pkt.chipIdLo() == 0x00100000u);
        REQUIRE(pkt.chipIdHi() == 0x00000001u);
        REQUIRE(pkt.serialNum() == 0x12345678u);
        REQUIRE(pkt.msmId() == 0x00070001u);
        REQUIRE(pkt.oemId() == 0x00000001u);
        REQUIRE(pkt.modelId() == 0x00000005u);
        REQUIRE(pkt.isV3());
    }

    SECTION("testCommandReadyRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::CommandReadyPacket original;
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        REQUIRE(parRes.value()->command() == 0x0Bu);
    }

    SECTION("testCommandSwitchModeRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::CommandSwitchModePacket original(1);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::CommandSwitchModePacket&>(*parRes.value());
        REQUIRE(pkt.mode() == 1u);
    }

    SECTION("testCommandExecRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::CommandExecPacket original(1);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::CommandExecPacket&>(*parRes.value());
        REQUIRE(pkt.clientCmd() == 1u);
    }

    SECTION("testCommandExecResponseRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::CommandExecResponsePacket original(1, 4);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::CommandExecResponsePacket&>(*parRes.value());
        REQUIRE(pkt.clientCmd() == 1u);
        REQUIRE(pkt.respLength() == 4u);
    }

    SECTION("testExecuteDataRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ExecuteDataPacket original(1, 1024);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::ExecuteDataPacket&>(*parRes.value());
        REQUIRE(pkt.clientCmd() == 1u);
        REQUIRE(pkt.dataLength() == 1024u);
    }

    SECTION("testExecuteDataResponseRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ExecuteDataResponsePacket original(1, 0);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::ExecuteDataResponsePacket&>(*parRes.value());
        REQUIRE(pkt.clientCmd() == 1u);
        REQUIRE(pkt.status() == 0u);
    }

    SECTION("testRxDataRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::RxDataPacket original(0, 0x1000, 64);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::RxDataPacket&>(*parRes.value());
        REQUIRE(pkt.imageId() == 0u);
        REQUIRE(pkt.dataOffset() == 0x1000u);
        REQUIRE(pkt.dataLength() == 64u);
    }

    SECTION("testRxDataResponseRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::RxDataResponsePacket original(0, 0);
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        auto& pkt = static_cast<const mbootcore::RxDataResponsePacket&>(*parRes.value());
        REQUIRE(pkt.imageId() == 0u);
        REQUIRE(pkt.status() == 0u);
    }

    SECTION("testResetStateMachineRoundTrip")
    {
        mbootcore::SaharaPacketSerializer ser;
        mbootcore::SaharaPacketParser parser;
        mbootcore::ResetStateMachinePacket original;
        auto serRes = ser.serialize(original);
        REQUIRE(serRes.isOk());
        auto parRes = parser.parse(serRes.value());
        REQUIRE(parRes.isOk());
        REQUIRE(parRes.value()->command() == 0x13u);
    }

    SECTION("testHelloRawBytes")
    {
        mbootcore::SaharaPacketSerializer ser;
        auto result = ser.serialize(mbootcore::HelloPacket(2, 2, 4096, 0));
        REQUIRE(result.isOk());
        auto& buf = result.value();
        REQUIRE(buf.size() == size_t(48));
        REQUIRE(buf[0] == 0x01); REQUIRE(buf[1] == 0x00);
        REQUIRE(buf[2] == 0x00); REQUIRE(buf[3] == 0x00);
        REQUIRE(buf[4] == 0x30); REQUIRE(buf[5] == 0x00);
        REQUIRE(buf[6] == 0x00); REQUIRE(buf[7] == 0x00);
        REQUIRE(buf[8] == 0x02); REQUIRE(buf[9] == 0x00);
        REQUIRE(buf[10] == 0x00); REQUIRE(buf[11] == 0x00);
        REQUIRE(buf[12] == 0x02); REQUIRE(buf[13] == 0x00);
        REQUIRE(buf[14] == 0x00); REQUIRE(buf[15] == 0x00);
        REQUIRE(buf[16] == 0x00); REQUIRE(buf[17] == 0x10);
        REQUIRE(buf[18] == 0x00); REQUIRE(buf[19] == 0x00);
        for (size_t i = 20; i < 48; ++i)
            REQUIRE(buf[i] == 0x00);
    }

    SECTION("testReadDataRawBytes")
    {
        mbootcore::SaharaPacketSerializer ser;
        auto result = ser.serialize(mbootcore::ReadDataPacket(0, 0x8000, 4096));
        REQUIRE(result.isOk());
        auto& buf = result.value();
        REQUIRE(buf.size() == size_t(20));
        REQUIRE(buf[0] == 0x03); REQUIRE(buf[1] == 0x00);
        REQUIRE(buf[2] == 0x00); REQUIRE(buf[3] == 0x00);
        REQUIRE(buf[4] == 0x14); REQUIRE(buf[5] == 0x00);
        REQUIRE(buf[6] == 0x00); REQUIRE(buf[7] == 0x00);
        REQUIRE(buf[8] == 0x00); REQUIRE(buf[9] == 0x00);
        REQUIRE(buf[10] == 0x00); REQUIRE(buf[11] == 0x00);
        REQUIRE(buf[12] == 0x00); REQUIRE(buf[13] == 0x80);
        REQUIRE(buf[14] == 0x00); REQUIRE(buf[15] == 0x00);
        REQUIRE(buf[16] == 0x00); REQUIRE(buf[17] == 0x10);
        REQUIRE(buf[18] == 0x00); REQUIRE(buf[19] == 0x00);
    }
}
