#include <catch2/catch_test_macros.hpp>

#include "mbootcore/core/protocols/firehose/FirehoseProtocol.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"
#include "mbootcore/logging/NullLogger.hpp"
#include "MockTransport.hpp"
#include "VirtualFirehoseDevice.hpp"

using namespace mbootcore;

namespace {
MockTransport m_transport;
NullLogger m_logger;
}

TEST_CASE("FirehoseVirtualTest", "[virtual]") {
    m_transport = MockTransport{};

SECTION("testConfigureSuccess")
{
    VirtualFirehoseDevice dev(m_transport);
    setupFirehoseSuccessResponses(dev);

    FirehoseProtocol proto(m_transport, m_logger);
    auto result = proto.handshake();
    REQUIRE(result.isOk());
    REQUIRE(proto.isConfigured());
}

SECTION("testConfigureFailure")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueNak("configure", "Unsupported memory type");

    FirehoseProtocol proto(m_transport, m_logger);
    auto result = proto.handshake();
    REQUIRE(result.isError());
    REQUIRE(!proto.isConfigured());
}

SECTION("testProgramSuccess")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueAck("program");

    FirehoseProtocol proto(m_transport, m_logger);
    auto handshakeResult = proto.handshake();
    REQUIRE(handshakeResult.isOk());

    ProgramCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 2;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    ByteBuffer data(8192, 0xAB);
    auto result = proto.program(cmd, data);
    REQUIRE(result.isOk());
}

SECTION("testProgramFailure")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueNak("program", "Write protected");

    FirehoseProtocol proto(m_transport, m_logger);
    auto handshakeResult = proto.handshake();
    REQUIRE(handshakeResult.isOk());

    ProgramCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;

    ByteBuffer data(4096, 0xCD);
    auto result = proto.program(cmd, data);
    REQUIRE(result.isError());
}

SECTION("testReadSuccess")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueReadAck();

    {
        ByteBuffer chunk(4096, 0xAA);
        m_transport.setReadData(chunk);
    }

    FirehoseProtocol proto(m_transport, m_logger);
    auto handshakeResult = proto.handshake();
    REQUIRE(handshakeResult.isOk());

    ReadCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 1;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    auto result = proto.read(cmd);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == size_t(4096));
}

SECTION("testEraseSuccess")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueAck("erase");

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    EraseCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 128;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    auto result = proto.erase(cmd);
    REQUIRE(result.isOk());
}

SECTION("testResetSuccess")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueAck("reset");

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());
    auto result = proto.reset();
    REQUIRE(result.isOk());
}

SECTION("testNopCommand")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueConfigureAck(ConfigureCommand{});
    dev.queueAck("NOP");

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    NopCommand cmd;
    auto result = proto.sendCommand(cmd);
    REQUIRE(result.isOk());
    REQUIRE(result.value().isAck());
}

SECTION("testPeekCommand")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueAck("peek");

    FirehoseProtocol proto(m_transport, m_logger);

    PeekCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 256;

    auto result = proto.peek(cmd);
    REQUIRE(result.isOk());
}

SECTION("testPokeCommand")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.queueAck("poke");

    FirehoseProtocol proto(m_transport, m_logger);

    PokeCommand cmd;
    cmd.address = 0x80000000;
    cmd.size = 4;

    auto result = proto.poke(cmd);
    REQUIRE(result.isOk());
}

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

SECTION("testCancelDuringHandshake")
{
    VirtualFirehoseDevice dev(m_transport);

    FirehoseProtocol proto(m_transport, m_logger);
    proto.cancel();
    auto result = proto.handshake();
    REQUIRE(result.isError());
}

}
