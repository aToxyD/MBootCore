#include <catch2/catch_test_macros.hpp>

#include "mbootcore/core/protocols/firehose/FirehoseProtocol.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"
#include "mbootcore/core/protocols/firehose/ChunkEngine.hpp"
#include "mbootcore/logging/NullLogger.hpp"
#include "../mocks/MockTransport.hpp"
#include "../virtual/VirtualFirehoseDevice.hpp"

#include <random>

using namespace mbootcore;

namespace {
MockTransport m_transport;
NullLogger m_logger;
}

TEST_CASE("FirehoseStressTest", "[firehose]") {
    m_transport = MockTransport{};

SECTION("test1MBWrite")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(4096, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    ProgramCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 256;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    ByteBuffer data(1024 * 1024, 0xA5);
    dev.queueAck("program");
    auto result = proto.program(cmd, data);
    REQUIRE(result.isOk());
}

SECTION("test10MBWrite")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(25600, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    ProgramCommand cmd;
    cmd.sectorSize = 4096;
    cmd.numSectorSize = 2560;
    cmd.partitionId = 0;
    cmd.startSector = 0;

    ByteBuffer data(10 * 1024 * 1024, 0x5A);
    dev.queueAck("program");
    auto result = proto.program(cmd, data);
    REQUIRE(result.isOk());
}

SECTION("testRandomWrites")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(4096, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    std::mt19937 rng(42);
    for (int i = 0; i < 20; ++i) {
        uint32_t sector = rng() % 2048;
        uint32_t count = (rng() % 64) + 1;
        auto data = ByteBuffer(count * 4096, static_cast<uint8_t>(rng() & 0xFF));

        ProgramCommand cmd;
        cmd.startSector = sector;
        cmd.sectorSize = 4096;
        cmd.numSectorSize = count;
        cmd.partitionId = 0;

        dev.queueAck("program");
        auto result = proto.program(cmd, data);
        REQUIRE(result.isOk());
    }
}

SECTION("testRandomReads")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(4096, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    std::mt19937 rng(42);
    for (int i = 0; i < 20; ++i) {
        uint32_t sector = rng() % 2048;
        uint32_t count = (rng() % 32) + 1;

        ByteBuffer expected(count * 4096, static_cast<uint8_t>(rng() & 0xFF));
        dev.setSectorData(sector, expected);

        ReadCommand cmd;
        cmd.startSector = sector;
        cmd.sectorSize = 4096;
        cmd.numSectorSize = count;
        cmd.partitionId = 0;

        dev.queueReadAck();
        ByteBuffer readData(count * 4096, 0xBB);
        m_transport.setReadData(readData);

        auto result = proto.read(cmd);
        REQUIRE(result.isOk());
    }
}

SECTION("testEraseProgramCycles")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(1024, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    for (int cycle = 0; cycle < 10; ++cycle) {
        EraseCommand eraseCmd;
        eraseCmd.startSector = 0;
        eraseCmd.sectorSize = 4096;
        eraseCmd.numSectorSize = 128;
        eraseCmd.partitionId = 0;

        dev.queueAck("erase");
        REQUIRE(proto.erase(eraseCmd).isOk());

        ProgramCommand progCmd;
        progCmd.startSector = 0;
        progCmd.sectorSize = 4096;
        progCmd.numSectorSize = 128;
        progCmd.partitionId = 0;

        ByteBuffer data(128 * 4096, static_cast<uint8_t>(cycle));
        dev.queueAck("program");
        REQUIRE(proto.program(progCmd, data).isOk());
    }
}

SECTION("testBoundaryWrites")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(1024, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    struct TestCase {
        uint32_t startSector;
        uint32_t count;
        const char* label;
    };

    TestCase cases[] = {
        {0, 1, "first sector"},
        {1023, 1, "last sector"},
        {0, 1024, "all sectors"},
        {512, 512, "half boundary"},
        {0, 0, "zero sectors"},
    };

    for (const auto& tc : cases) {
        ProgramCommand cmd;
        cmd.startSector = tc.startSector;
        cmd.sectorSize = 4096;
        cmd.numSectorSize = tc.count;
        cmd.partitionId = 0;

        ByteBuffer data(tc.count * 4096, 0xFF);
        if (tc.count > 0) {
            dev.queueAck("program");
            auto result = proto.program(cmd, data);
            REQUIRE(result.isOk());
        }
    }
}

SECTION("testOverlappingWrites")
{
    VirtualFirehoseDevice dev(m_transport);
    dev.configureStorage(1024, 4096);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseProtocol proto(m_transport, m_logger);
    REQUIRE(proto.handshake().isOk());

    for (int i = 0; i < 5; ++i) {
        ProgramCommand cmd;
        cmd.startSector = static_cast<uint32_t>(i * 100);
        cmd.sectorSize = 4096;
        cmd.numSectorSize = 200;
        cmd.partitionId = 0;

        ByteBuffer data(200 * 4096, static_cast<uint8_t>(i));
        dev.queueAck("program");
        REQUIRE(proto.program(cmd, data).isOk());
    }
}

SECTION("testChunkEngine")
{
    ChunkEngine::Config cfg;
    cfg.chunkSize = 65536;
    cfg.chunkTimeout = std::chrono::milliseconds(1000);
    cfg.responseTimeout = std::chrono::milliseconds(500);
    cfg.maxRetries = 2;

    ChunkEngine engine(cfg);

    size_t totalSize = 1024 * 1024;
    ByteBuffer testData(totalSize, 0x42);

    size_t transferred = 0;
    auto writeFn = [&](const uint8_t* data, size_t size) -> Result<void> {
        (void)data;
        transferred += size;
        return {};
    };
    auto responseFn = []() -> Result<void> {
        return {};
    };
    auto progressFn = [](size_t done, size_t total) {
        REQUIRE(done <= total);
    };

    auto result = engine.streamToTarget(
        testData.data(), totalSize, writeFn, responseFn, progressFn);
    REQUIRE(result.isOk());
    REQUIRE(transferred == totalSize);
}

SECTION("testChunkEnginePartial")
{
    ChunkEngine::Config cfg;
    cfg.chunkSize = 4096;
    ChunkEngine engine(cfg);

    ByteBuffer data(65536, 0xAB);
    size_t received = 0;
    auto writeFn = [&](const uint8_t*, size_t size) -> Result<void> {
        received += size;
        return {};
    };

    auto result = engine.streamToTargetPartial(
        data.data(), data.size(), 8192, 16384, writeFn, nullptr);
    REQUIRE(result.isOk());
    REQUIRE(received == size_t(16384));
}

SECTION("testChunkEngineCancel")
{
    ChunkEngine::Config cfg;
    cfg.chunkSize = 4096;
    ChunkEngine engine(cfg);

    engine.cancel();

    ByteBuffer data(65536, 0x00);
    auto writeFn = [](const uint8_t*, size_t) -> Result<void> {
        return {};
    };

    auto result = engine.streamToTarget(data.data(), data.size(), writeFn, nullptr);
    REQUIRE(result.isError());
}

SECTION("testChunkEngineResumeOffset")
{
    ChunkEngine::Config cfg;
    cfg.chunkSize = 4096;
    ChunkEngine engine(cfg);

    REQUIRE(engine.resumeOffset() == size_t(0));

    ByteBuffer data(8192, 0xCD);
    size_t received = 0;
    auto writeFn = [&](const uint8_t*, size_t size) -> Result<void> {
        received += size;
        return {};
    };

    auto result = engine.streamToTarget(data.data(), data.size(), writeFn, nullptr);
    REQUIRE(result.isOk());
    REQUIRE(engine.resumeOffset() == data.size());
}

}
