#include <catch2/catch_test_macros.hpp>

#include "mbootcore/gpt/GPTWriter.hpp"
#include "mbootcore/gpt/GPTModels.hpp"
#include "mbootcore/gpt/Guid.hpp"
#include "../mocks/MockFlashDevice.hpp"

#include <zlib.h>
#include <cstring>
#include <limits>

using namespace mbootcore::gpt;
using mbootcore::ByteBuffer;
using mbootcore::ErrorCode;

namespace {

// writeLE32 intentionally unused; kept for reference

uint32_t readLE32(const ByteBuffer& buf, size_t off) {
    return static_cast<uint32_t>(buf[off]) |
          (static_cast<uint32_t>(buf[off+1]) << 8) |
          (static_cast<uint32_t>(buf[off+2]) << 16) |
          (static_cast<uint32_t>(buf[off+3]) << 24);
}

}

TEST_CASE("GPTWriterTest", "[gpt]") {

SECTION("testWritePrimaryHeader") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    header.diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    header.partitionEntryLBA = 2;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;

    auto crcResult = writer.updateHeaderCRC(header);
    REQUIRE(crcResult.isOk());

    auto result = writer.writePrimaryHeader(header);
    REQUIRE(result.isOk());

    auto readResult = dev.readMemory(512, 512);
    REQUIRE(readResult.isOk());
    const auto& data = readResult.value();

    char sig[9] = {};
    std::memcpy(sig, data.data(), 8);
    REQUIRE(std::string_view(sig, 8) == "EFI PART");

    REQUIRE(readLE32(data, 8) == 0x00010000u);
    REQUIRE(readLE32(data, 12) == 92u);
    REQUIRE(readLE32(data, 16) != 0);
}

SECTION("testWriteBackupHeader") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    header.diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    header.partitionEntryLBA = 2;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;

    auto result = writer.writeBackupHeader(header);
    REQUIRE(result.isOk());

    auto readResult = dev.readMemory(1023 * 512, 512);
    REQUIRE(readResult.isOk());
    const auto& data = readResult.value();

    char sig[9] = {};
    std::memcpy(sig, data.data(), 8);
    REQUIRE(std::string_view(sig, 8) == "EFI PART");

    REQUIRE(readLE32(data, 12) == 92u);
}

SECTION("testWritePartitionEntries") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    header.partitionEntryLBA = 2;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;

    std::vector<PartitionEntry> entries(128);
    entries[0].partitionTypeGUID = PartitionTypes::EFISystem;
    entries[0].uniquePartitionGUID = Guid::fromString("11111111-2222-3333-4444-555555555555");
    entries[0].firstLBA = 34;
    entries[0].lastLBA = 100;
    char16_t name[] = u"boot";
    std::copy(name, name + 5, entries[0].name.begin());

    auto result = writer.writePartitionEntries(header, entries);
    REQUIRE(result.isOk());

    auto readResult = dev.readMemory(2 * 512, 16384);
    REQUIRE(readResult.isOk());
    const auto& data = readResult.value();

    uint32_t type1 = readLE32(data, 0);
    REQUIRE(type1 == 0xC12A7328u);
}

SECTION("testUpdateHeaderCRC") {
    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    header.diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    header.partitionEntryLBA = 2;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;

    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    REQUIRE(header.headerCrc32 == 0u);
    auto result = writer.updateHeaderCRC(header);
    REQUIRE(result.isOk());
    REQUIRE(header.headerCrc32 != 0);
}

SECTION("testWritePrimaryAndBackup") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    header.diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    header.partitionEntryLBA = 2;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;

    std::vector<PartitionEntry> entries(128);
    entries[0].partitionTypeGUID = PartitionTypes::EFISystem;
    entries[0].uniquePartitionGUID = Guid::fromString("11111111-2222-3333-4444-555555555555");
    entries[0].firstLBA = 34;
    entries[0].lastLBA = 100;
    char16_t name[] = u"boot";
    std::copy(name, name + 5, entries[0].name.begin());

    auto result = writer.writePrimaryAndBackup(header, entries);
    REQUIRE(result.isOk());

    auto primaryRead = dev.readMemory(512, 512);
    REQUIRE(primaryRead.isOk());
    REQUIRE(primaryRead.value()[0] == 'E');

    auto backupRead = dev.readMemory(1023 * 512, 512);
    REQUIRE(backupRead.isOk());
    REQUIRE(backupRead.value()[0] == 'E');

    auto entryRead = dev.readMemory(2 * 512, 16384);
    REQUIRE(entryRead.isOk());
    REQUIRE(readLE32(entryRead.value(), 0) == 0xC12A7328u);
}

SECTION("testComputeHeaderCRC") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;

    auto result = writer.computeHeaderCRC(header);
    REQUIRE(result.isOk());
    REQUIRE(result.value() != 0);

    header.firstUsableLBA = 34;
    auto result2 = writer.computeHeaderCRC(header);
    REQUIRE(result2.isOk());
    REQUIRE(result2.value() != result.value());
}

SECTION("testComputeEntryCRC") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);

    std::vector<PartitionEntry> entries(128);
    entries[0].partitionTypeGUID = PartitionTypes::EFISystem;
    entries[0].firstLBA = 34;
    entries[0].lastLBA = 100;

    auto result = writer.computeEntryCRC(entries, 128, 128);
    REQUIRE(result.isOk());
    REQUIRE(result.value() != 0);

    std::vector<PartitionEntry> nulls(128);
    auto result2 = writer.computeEntryCRC(nulls, 128, 128);
    REQUIRE(result2.isOk());
    auto result3 = writer.computeEntryCRC(nulls, 128, 128);
    REQUIRE(result3.isOk());
    REQUIRE(result2.value() == result3.value());
}

SECTION("testLayoutAccessor") {
    MockFlashDevice dev(2048, 512);
    GPTWriter writer(dev);
    auto layout = writer.layout();

    REQUIRE(layout.sectorSize == 512u);
    REQUIRE(layout.totalSectors == 2048ULL);
    REQUIRE(layout.primaryHeaderLBA == 1ULL);
    REQUIRE(layout.backupHeaderLBA == 2047ULL);
    REQUIRE(layout.backupEntriesLBA == 2015ULL);
}

SECTION("testWriteSectorOverflow") {
    MockFlashDevice dev(1024, 512);
    GPTWriter writer(dev);
    GPTHeader header;
    header.signature = GPTHeader::Signature;
    header.revision = GPTHeader::Revision;
    header.headerSize = 92;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    // partitionEntryLBA chosen so lba * 512 overflows uint64_t
    header.partitionEntryLBA = std::numeric_limits<uint64_t>::max() / 2;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;
    std::vector<PartitionEntry> entries(128);
    auto result = writer.writePartitionEntries(header, entries);
    REQUIRE(result.isError());
}

}
