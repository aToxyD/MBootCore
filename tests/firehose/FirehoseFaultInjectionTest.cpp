#include <catch2/catch_test_macros.hpp>

#include "mbootcore/core/protocols/firehose/FirehoseProtocol.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"
#include "mbootcore/core/protocols/firehose/FlashContext.hpp"
#include "mbootcore/generic/StorageInfo.hpp"
#include "mbootcore/logging/NullLogger.hpp"
#include "../mocks/MockTransport.hpp"
#include "../virtual/VirtualFirehoseDevice.hpp"

using namespace mbootcore;

namespace {
MockTransport m_transport;
NullLogger m_logger;
}

TEST_CASE("FirehoseFaultInjectionTest", "[firehose]") {
    m_transport = MockTransport{};

SECTION("testNakResponse")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueNak("erase", "Sector locked");

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    EraseCommand cmd;
    cmd.partitionId = 0;
    cmd.startSector = 100;

    auto result = proto.erase(cmd);
    REQUIRE(result.isError());
}

SECTION("testNakAllMode")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(1024, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    dev.queueNak("erase", "Simulated NAK all");

    EraseCommand cmd;
    cmd.startSector = 0;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 10;
    cmd.partitionId = 0;

    auto result = proto.erase(cmd);
    REQUIRE(result.isError());
}

SECTION("testShortResponse")
{
    ByteBuffer shortData(64, 0xBB);
    m_transport.setReadData(shortData);
    m_transport.setReadResult(Result<size_t>(ErrorCode::TransportError));

    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    ReadCommand cmd;
    cmd.startSector = 0;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 4;
    cmd.partitionId = 0;

    dev.queueReadAck();

    auto result = proto.read(cmd);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() > 0);
}

SECTION("testDisconnect")
{
    m_transport.setReadResult(
        Result<size_t>(ErrorCode::TransportDisconnected));

    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    ByteBuffer normalBuf(8192);
    m_transport.setReadData(normalBuf);

    EraseCommand cmd;
    cmd.startSector = 0;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 10;
    cmd.partitionId = 0;

    auto result = proto.erase(cmd);
    REQUIRE(result.isOk());
}

SECTION("testInvalidXml")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    ByteBuffer invalidBuf(
        {'<','<','i','n','v','a','l','i','d','>','>',0});
    m_transport.setReadData(invalidBuf);

    NopCommand cmd;
    auto result = proto.sendCommand(cmd);
    REQUIRE(!result.value().isAck());
}

SECTION("testPartialWrite")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(1024, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    dev.queueNak("program", "Partial write - incomplete");

    ProgramCommand cmd;
    cmd.startSector = 0;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;
    cmd.partitionId = 0;

    ByteBuffer data(4096, 0xAA);
    auto result = proto.program(cmd, data);
    REQUIRE(result.isError());
}

SECTION("testBadBlock")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueNak("erase", "Bad block at sector 5");

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    EraseCommand cmd;
    cmd.startSector = 5;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;
    cmd.partitionId = 0;

    auto result = proto.erase(cmd);
    REQUIRE(result.isError());
}

SECTION("testRandomNakMode")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    int nakCount = 0;
    for (int i = 0; i < 20; ++i) {
        EraseCommand cmd;
        cmd.startSector = static_cast<uint32_t>(i * 10);
        cmd.sectorSize = 4096;
        cmd.numSectorSize = 1;
        cmd.partitionId = 0;

        if (i % 3 == 0) {
            dev.queueNak("erase", "Random fault");
        } else {
            dev.queueAck("erase");
        }

        auto result = proto.erase(cmd);
        if (result.isError()) ++nakCount;
    }
    REQUIRE(nakCount > 0);
    REQUIRE(nakCount < 15);
}

SECTION("testTimeoutSimulation")
{
    MockTransport slowTransport;
    slowTransport.setReadResult(
        Result<size_t>(ErrorCode::TransportTimeout));

    VirtualFirehoseDevice dev(slowTransport);
    dev.configureStorage(1024, 4096);

    FirehoseProtocol proto(slowTransport, m_logger);
    auto result = proto.handshake();
    REQUIRE(result.isError());
}

SECTION("testConsecutiveEraseProgramCycles")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(256, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    for (int i = 0; i < 50; ++i) {
        dev.queueAck("erase");
        dev.queueAck("program");

        EraseCommand eraseCmd;
        eraseCmd.startSector = static_cast<uint32_t>(i * 0);
        eraseCmd.sectorSize = 4096;
        eraseCmd.numSectorSize = 1;
        eraseCmd.partitionId = 0;
        REQUIRE(proto.erase(eraseCmd).isOk());

        ProgramCommand progCmd;
        progCmd.startSector = static_cast<uint32_t>(i * 0);
        progCmd.sectorSize = 4096;
        progCmd.numSectorSize = 1;
        progCmd.partitionId = 0;

        ByteBuffer data(4096, static_cast<uint8_t>(i));
        auto result = proto.program(progCmd, data);
        REQUIRE(result.isOk());
    }
}

SECTION("testPokeDataSerialization")
{
    PokeCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 8;
    cmd.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    auto xml = cmd.serialize();
    REQUIRE(!xml.empty());
    REQUIRE(xml.find("data=\"0102030405060708\"") != std::string::npos);
}

SECTION("testXmlEntityEscaping")
{
    XmlElement elem;
    elem.name = "test";
    elem.attributes = {
        {"attr1", "hello & goodbye"},
        {"attr2", "less < than > & \"quoted\""},
    };

    auto result = FirehoseXmlEngine::serialize(elem);
    REQUIRE(result.isOk());
    auto xml = result.value();
    REQUIRE(xml.find("&amp;") != std::string::npos);
    REQUIRE(xml.find("&lt;") != std::string::npos);
    REQUIRE(xml.find("&gt;") != std::string::npos);
    REQUIRE(xml.find("&quot;") != std::string::npos);
}

SECTION("testFlashContext")
{
    FlashContext ctx;
    REQUIRE(ctx.memoryName == "ufs");
    REQUIRE(ctx.storageType == StorageType::UFS);
    REQUIRE(ctx.sectorSize == 4096);
    REQUIRE(ctx.maxPayloadToTarget == 1048576);
    REQUIRE(ctx.eraseSupported);

    ctx.memoryName = "emmc";
    ctx.storageType = StorageType::eMMC;
    ctx.sectorSize = 512;
    ctx.maxPayloadToTarget = 65536;
    ctx.eraseSupported = false;

    REQUIRE(ctx.memoryName == "emmc");
    REQUIRE(ctx.storageType == StorageType::eMMC);
    REQUIRE(ctx.sectorSize == 512);
    REQUIRE(ctx.maxPayloadToTarget == 65536);
    REQUIRE(!ctx.eraseSupported);
}

SECTION("peek_transportWriteFailure")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    m_transport.setWriteResult(
        Result<size_t>::Error(ErrorCode::TransportWriteFailed));

    PeekCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 4;
    auto result = proto.peek(cmd);
    REQUIRE(result.isError());
}

SECTION("peek_cancelledBeforeExecution")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    proto.cancel();

    PeekCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 4;
    auto result = proto.peek(cmd);
    REQUIRE(result.error() == ErrorCode::Cancelled);
}

SECTION("peek_transportReadFailure")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    m_transport.clearReadQueue();
    m_transport.setReadResult(
        Result<size_t>::Error(ErrorCode::TransportError));

    PeekCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 4;
    auto result = proto.peek(cmd);
    REQUIRE(result.isError());
}

SECTION("reset_malformedResponse")
{
    ByteBuffer garbage(
        {'<', '<', 'i', 'n', 'v', 'a', 'l', 'i', 'd', '>', '>'});
    m_transport.setReadData(garbage);

    FirehoseProtocol proto(m_transport, m_logger);
    auto result = proto.reset();
    REQUIRE(result.isError());
}

SECTION("configure_retryExhaustion")
{
    std::string nakXml =
        "<configure value=\"NAK\" description=\"Try again\"/>";
    ByteBuffer nak1(nakXml.begin(), nakXml.end());
    ByteBuffer nak2(nakXml.begin(), nakXml.end());
    ByteBuffer nak3(nakXml.begin(), nakXml.end());

    m_transport.setReadData(nak1);
    m_transport.setReadData(nak2);
    m_transport.setReadData(nak3);

    FirehoseProtocol proto(m_transport, m_logger);
    auto result = proto.handshake();
    REQUIRE(result.error() == ErrorCode::ProtocolError);
}

SECTION("peek_nakResponse")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("peek", "Access denied");

    FirehoseProtocol proto(m_transport, m_logger);
    PeekCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 4;
    auto result = proto.peek(cmd);
    REQUIRE(result.isError());
}

SECTION("poke_nakResponse")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("poke", "Write protected");

    FirehoseProtocol proto(m_transport, m_logger);
    PokeCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 4;
    cmd.data = {0xDE, 0xAD, 0xBE, 0xEF};
    auto result = proto.poke(cmd);
    REQUIRE(result.isError());
}

SECTION("patch_nakResponse")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("patch", "Invalid element");

    FirehoseProtocol proto(m_transport, m_logger);
    PatchCommand cmd;
    cmd.byteOffset = 0x100;
    cmd.size = 4;
    cmd.data = 0xCAFEBABE;
    cmd.filename = "image.bin";
    auto result = proto.patch(cmd);
    REQUIRE(result.isError());
}

SECTION("getSha256Digest_nakResponse")
{
    // Current behavior:
    // getSha256Digest() does not treat Firehose NAK as an error.
    // A NAK response is returned as an empty digest.
    // This test documents the current behavior rather than endorsing it.

    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("getsha256digest", "Access denied");

    FirehoseProtocol proto(m_transport, m_logger);
    GetSha256DigestCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;
    auto result = proto.getSha256Digest(cmd);
    REQUIRE(result.isOk());
    REQUIRE(result.value().empty());
}

SECTION("program_invalidState")
{
    FirehoseProtocol proto(m_transport, m_logger);

    ProgramCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    ByteBuffer data(4096, 0xAB);
    auto result = proto.program(cmd, data);
    REQUIRE(result.error() == ErrorCode::InvalidState);
}

SECTION("read_invalidState")
{
    FirehoseProtocol proto(m_transport, m_logger);

    ReadCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    auto result = proto.read(cmd);
    REQUIRE(result.error() == ErrorCode::InvalidState);
}

SECTION("configureMemory_nakResponse")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("configurememory", "Invalid config");

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE_FALSE(proto.isConfigured());

    ConfigureMemoryCommand cmd;
    cmd.memoryName = "ufs";
    cmd.config = "all";
    auto result = proto.configureMemory(cmd);
    REQUIRE(result.isError());
    REQUIRE_FALSE(proto.isConfigured());
}

SECTION("uploadProgrammer_notSupported")
{
    FirehoseProtocol proto(m_transport, m_logger);
    ByteBuffer dummy(256, 0xFF);
    auto result = proto.uploadProgrammer(dummy);
    REQUIRE(result.error() == ErrorCode::NotSupported);
}

SECTION("getStorageInfo_nakResponse")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("getstorageinfo", "Not available");

    FirehoseProtocol proto(m_transport, m_logger);
    auto result = proto.getStorageInfo();
    REQUIRE(result.isOk());
    REQUIRE(result.value().isNak());
}

SECTION("powerReset_nakResponse")
{
    // Current behavior:
    // powerReset() does not distinguish ACK from NAK.
    // Any successfully parsed Firehose response is treated as success.
    // This test documents the current behavior rather than endorsing it.

    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("power", "Not permitted");

    FirehoseProtocol proto(m_transport, m_logger);
    PowerCommand cmd;
    auto result = proto.powerReset(cmd);
    REQUIRE(result.isOk());
}

}
