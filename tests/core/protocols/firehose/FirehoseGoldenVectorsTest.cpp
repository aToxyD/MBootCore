#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"

using namespace mbootcore;

namespace {

void runConfigureRoundTrip() {
    ConfigureCommand original;
    original.memoryName = "ufs";
    original.zlpAwareHost = 1;
    original.maxPayloadSizeToTarget = 1048576;
    original.maxPayloadSizeFromTarget = 1048576;
    original.mode = 0;

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<ConfigureCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->memoryName == "ufs");
    REQUIRE(cmd->maxPayloadSizeToTarget == 1048576u);
    REQUIRE(cmd->mode == 0u);
}

void runProgramRoundTrip() {
    ProgramCommand original;
    original.sectorSize = 4096;
    original.numSectorSize = 512;
    original.partitionId = 0;
    original.startSector = 0;
    original.physicalPartition = 0;
    original.numPartitionSectors = 2048;
    original.filename = "image.bin";
    original.label = "system_a";

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<ProgramCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->sectorSize == 4096u);
    REQUIRE(cmd->numSectorSize == 512u);
    REQUIRE(cmd->partitionId == 0u);
    REQUIRE(cmd->startSector == 0u);
    REQUIRE(cmd->filename == "image.bin");
    REQUIRE(cmd->label == "system_a");
}

void runReadRoundTrip() {
    ReadCommand original;
    original.sectorSize = 4096;
    original.numSectorSize = 32;
    original.partitionId = 0;
    original.startSector = 1000;
    original.physicalPartition = 0;

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<ReadCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->sectorSize == 4096u);
    REQUIRE(cmd->numSectorSize == 32u);
    REQUIRE(cmd->startSector == 1000u);
}

void runEraseRoundTrip() {
    EraseCommand original;
    original.sectorSize = 4096;
    original.numSectorSize = 128;
    original.partitionId = 0;
    original.startSector = 0;

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<EraseCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->sectorSize == 4096u);
    REQUIRE(cmd->numSectorSize == 128u);
}

void runPeekRoundTrip() {
    PeekCommand original;
    original.address = 0x80000000;
    original.size = 256;

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<PeekCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->address == 0x80000000u);
    REQUIRE(cmd->size == 256u);
}

void runPokeRoundTrip() {
    PokeCommand original;
    original.address = 0x80000000;
    original.size = 256;
    original.data = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<PokeCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->address == 0x80000000u);
    REQUIRE(cmd->size == 256u);
    REQUIRE(cmd->data == original.data);
}

void runPatchRoundTrip() {
    PatchCommand original;
    original.byteOffset = 0x100;
    original.size = 4;
    original.data = 0xCAFEBABE;
    original.filename = "image.bin";

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<PatchCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->byteOffset == 0x100u);
    REQUIRE(cmd->data == 0xCAFEBABEu);
}

void runResetRoundTrip() {
    ResetCommand original;
    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<ResetCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->value == "reset");
}

void runNopRoundTrip() {
    NopCommand original;
    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    REQUIRE(dynamic_cast<NopCommand*>(parsed.value().get()) != nullptr);
}

void runGetStorageInfoRoundTrip() {
    GetStorageInfoCommand original;
    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    REQUIRE(dynamic_cast<GetStorageInfoCommand*>(parsed.value().get()) != nullptr);
}

void runGetSha256DigestRoundTrip() {
    GetSha256DigestCommand original;
    original.sectorSize = 4096;
    original.numSectorSize = 32;

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<GetSha256DigestCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->sectorSize == 4096u);
    REQUIRE(cmd->numSectorSize == 32u);
}

void runConfigureMemoryRoundTrip() {
    ConfigureMemoryCommand original;
    original.memoryName = "ufs";
    original.config = "all";

    auto xml = original.serialize();
    auto parsed = parseFirehoseXml(xml);
    REQUIRE(parsed.isOk());

    auto* cmd = dynamic_cast<ConfigureMemoryCommand*>(parsed.value().get());
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->memoryName == "ufs");
    REQUIRE(cmd->config == "all");
}

} // anonymous namespace

TEST_CASE("FirehoseGoldenVectorsTest", "[firehose]") {
    SECTION("testConfigureRoundTrip") {
        runConfigureRoundTrip();
    }

    SECTION("testProgramRoundTrip") {
        runProgramRoundTrip();
    }

    SECTION("testReadRoundTrip") {
        runReadRoundTrip();
    }

    SECTION("testEraseRoundTrip") {
        runEraseRoundTrip();
    }

    SECTION("testPeekRoundTrip") {
        runPeekRoundTrip();
    }

    SECTION("testPokeRoundTrip") {
        runPokeRoundTrip();
    }

    SECTION("testPatchRoundTrip") {
        runPatchRoundTrip();
    }

    SECTION("testResetRoundTrip") {
        runResetRoundTrip();
    }

    SECTION("testNopRoundTrip") {
        runNopRoundTrip();
    }

    SECTION("testGetStorageInfoRoundTrip") {
        runGetStorageInfoRoundTrip();
    }

    SECTION("testGetSha256DigestRoundTrip") {
        runGetSha256DigestRoundTrip();
    }

    SECTION("testConfigureMemoryRoundTrip") {
        runConfigureMemoryRoundTrip();
    }

    SECTION("testResponseAckParsing") {
        std::string ackXml = "<program value=\"ACK\"/>";
        auto resp = FirehoseResponse::fromXml(ackXml);
        REQUIRE(resp.isAck());
        REQUIRE(!resp.isNak());
        REQUIRE(resp.commandName() == "program");
    }

    SECTION("testResponseNakParsing") {
        std::string nakXml = "<program value=\"NAK\" description=\"Sector out of range\"/>";
        auto resp = FirehoseResponse::fromXml(nakXml);
        REQUIRE(resp.isNak());
        REQUIRE(!resp.isAck());
        REQUIRE(resp.nakDescription() == "Sector out of range");
    }

    SECTION("testAllCommandsRoundTrip") {
        runConfigureRoundTrip();
        runProgramRoundTrip();
        runReadRoundTrip();
        runEraseRoundTrip();
        runPeekRoundTrip();
        runPokeRoundTrip();
        runPatchRoundTrip();
        runResetRoundTrip();
        runNopRoundTrip();
        runGetStorageInfoRoundTrip();
        runGetSha256DigestRoundTrip();
        runConfigureMemoryRoundTrip();
    }
}
