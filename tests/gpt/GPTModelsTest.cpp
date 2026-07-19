#include <catch2/catch_test_macros.hpp>

#include "mbootcore/gpt/Guid.hpp"
#include "mbootcore/gpt/GPTModels.hpp"
#include <limits>

using namespace mbootcore::gpt;

TEST_CASE("GPTModelsTest", "[gpt]") {

SECTION("testGuidNull") {
    Guid g;
    REQUIRE(g.isNull());

    Guid g2{0, 0, 0, {}};
    REQUIRE(g2.isNull());
}

SECTION("testGuidFromString") {
    auto g = Guid::fromString("00112233-4455-6677-8899-AABBCCDDEEFF");
    REQUIRE(g.data1 == 0x00112233u);
    REQUIRE(g.data2 == 0x4455u);
    REQUIRE(g.data3 == 0x6677u);
    REQUIRE(g.data4[0] == 0x88u);
    REQUIRE(g.data4[1] == 0x99u);
    REQUIRE(g.data4[2] == 0xAAu);
    REQUIRE(g.data4[3] == 0xBBu);
    REQUIRE(g.data4[4] == 0xCCu);
    REQUIRE(g.data4[5] == 0xDDu);
    REQUIRE(g.data4[6] == 0xEEu);
    REQUIRE(g.data4[7] == 0xFFu);

    auto nullG = Guid::fromString("00000000-0000-0000-0000-000000000000");
    REQUIRE(nullG.isNull());

    auto withBraces = Guid::fromString("{00112233-4455-6677-8899-AABBCCDDEEFF}");
    REQUIRE(!withBraces.isNull());
    REQUIRE(withBraces.data1 == 0x00112233u);
}

SECTION("testGuidToString") {
    Guid g{0x00112233, 0x4455, 0x6677,
           {0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
    std::string s = g.toString();
    REQUIRE(s == "00112233-4455-6677-8899-AABBCCDDEEFF");
}

SECTION("testGuidComparison") {
    Guid a{0x00112233, 0x4455, 0x6677,
           {0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
    Guid b = a;
    REQUIRE(a == b);
    REQUIRE(!(a != b));

    Guid c;
    REQUIRE(a != c);
    REQUIRE(!(a == c));
}

SECTION("testGuidHash") {
    Guid a{0x00112233, 0x4455, 0x6677,
           {0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
    Guid b = a;
    std::hash<Guid> hasher;
    REQUIRE(hasher(a) == hasher(b));
}

SECTION("testPartitionEntryIsUsed") {
    PartitionEntry entry;
    REQUIRE(!entry.isUsed());

    entry.partitionTypeGUID = PartitionTypes::EFISystem;
    REQUIRE(entry.isUsed());

    entry.partitionTypeGUID = Guid::Null;
    REQUIRE(!entry.isUsed());
}

SECTION("testPartitionEntrySize") {
    PartitionEntry entry;
    entry.firstLBA = 100;
    entry.lastLBA = 199;
    REQUIRE(entry.sizeInLBAs() == 100ULL);
    REQUIRE(entry.sizeInBytes() == 51200ULL);

    entry.firstLBA = 0;
    entry.lastLBA = 0;
    REQUIRE(entry.sizeInLBAs() == 0ULL);
    REQUIRE(entry.sizeInBytes() == 0ULL);

    entry.firstLBA = 34;
    entry.lastLBA = 33;
    REQUIRE(entry.sizeInLBAs() == 0ULL);
}

SECTION("testPartitionEntryNameConversion") {
    std::array<char16_t, 36> name{};
    name[0] = u'b'; name[1] = u'o'; name[2] = u'o'; name[3] = u't'; name[4] = 0;
    PartitionEntry entry;
    entry.name = name;
    REQUIRE(entry.nameToString() == "boot");

    auto u16 = PartitionEntry::utf8ToUtf16("test_partition");
    REQUIRE(u16.size() == 14ULL);
    auto back = PartitionEntry::utf16ToUtf8(u16.data(), u16.size());
    REQUIRE(back == "test_partition");
}

SECTION("testGPTHeaderValidation") {
    GPTHeader header;
    REQUIRE(!header.isValid());

    header.signature = GPTHeader::Signature;
    REQUIRE(!header.isValid());

    header.revision = GPTHeader::Revision;
    header.headerSize = GPTHeader::MinHeaderSize;
    header.myLBA = 1;
    header.alternateLBA = 1023;
    header.firstUsableLBA = 34;
    header.lastUsableLBA = 989;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;
    REQUIRE(header.isValid());
    REQUIRE(header.hasValidSignature());

    header.signature = 0;
    REQUIRE(!header.isValid());
    REQUIRE(!header.hasValidSignature());
}

SECTION("testGPTHeaderEntrySize") {
    GPTHeader header;
    header.numberOfPartitionEntries = 128;
    header.sizeOfPartitionEntry = 128;
    REQUIRE(header.totalEntrySize() == 16384ULL);

    header.partitionEntryLBA = 2;
    REQUIRE(header.lastEntryLBA() == 33ULL);
}

SECTION("testGPTLayoutCalculation") {
    GPTLayout layout;
    layout.totalSectors = 1024;
    layout.sectorSize = 512;
    layout.firstUsableLBA = 34;
    layout.lastUsableLBA = 989;
    REQUIRE(layout.isValid());
    REQUIRE(layout.firstUsableByte() == 17408ULL);
    REQUIRE(layout.lastUsableByte() == 506368ULL);
    REQUIRE(layout.totalUsableSectors() == 956ULL);

    GPTLayout emptyLayout;
    REQUIRE(!emptyLayout.isValid());
}

SECTION("testPartitionRange") {
    PartitionRange range{100, 199};
    REQUIRE(range.sectorCount() == 100ULL);
    REQUIRE(range.byteCount() == 51200ULL);
    REQUIRE(range.contains(100));
    REQUIRE(range.contains(149));
    REQUIRE(range.contains(199));
    REQUIRE(!range.contains(99));
    REQUIRE(!range.contains(200));
}

SECTION("testPartitionRangeOverlap") {
    PartitionRange a{100, 199};
    PartitionRange b{150, 249};
    PartitionRange c{200, 299};
    PartitionRange d{300, 399};

    REQUIRE(a.overlaps(b));
    REQUIRE(b.overlaps(a));
    REQUIRE(!a.overlaps(c));
    REQUIRE(!a.overlaps(d));
    REQUIRE(!c.overlaps(a));
    REQUIRE(!c.overlaps(a));
}

SECTION("testPartitionTypeConstants") {
    REQUIRE(PartitionTypes::Unused.isNull());
    REQUIRE(!PartitionTypes::EFISystem.isNull());
    REQUIRE(!PartitionTypes::BasicData.isNull());
    REQUIRE(!PartitionTypes::AndroidBoot.isNull());
    REQUIRE(!PartitionTypes::QualcommDump.isNull());

    auto efi = Guid::fromString("C12A7328-F81F-11D2-BA4B-00A0C93EC93B");
    REQUIRE(PartitionTypes::EFISystem == efi);
}

SECTION("testPartitionAttributes") {
    uint64_t attrs = static_cast<uint64_t>(PartitionAttribute::RequiredPartition);
    REQUIRE(hasAttribute(attrs, PartitionAttribute::RequiredPartition));
    REQUIRE(!hasAttribute(attrs, PartitionAttribute::NoBlockIOProtocol));

    attrs |= static_cast<uint64_t>(PartitionAttribute::NoBlockIOProtocol);
    REQUIRE(hasAttribute(attrs, PartitionAttribute::RequiredPartition));
    REQUIRE(hasAttribute(attrs, PartitionAttribute::NoBlockIOProtocol));

    REQUIRE(hasAttribute(static_cast<uint64_t>(PartitionAttribute::AndroidShouldFlash),
                          PartitionAttribute::AndroidShouldFlash));
}

SECTION("testGPTTableIsValid") {
    GPTTable table;
    REQUIRE(!table.isValid());

    table.primaryValid = true;
    REQUIRE(table.isValid());

    table.primaryValid = false;
    table.backupValid = true;
    REQUIRE(table.isValid());
}

SECTION("testGPTTableUsedCount") {
    GPTTable table;
    REQUIRE(table.usedEntryCount() == 0ULL);

    table.entries.resize(4);
    table.entries[0].partitionTypeGUID = PartitionTypes::EFISystem;
    table.entries[2].partitionTypeGUID = PartitionTypes::BasicData;
    REQUIRE(table.usedEntryCount() == 2ULL);
}

SECTION("testSizeInBytesOverflow") {
    PartitionEntry entry;
    entry.partitionTypeGUID = PartitionTypes::EFISystem;
    entry.firstLBA = 1;
    entry.lastLBA = std::numeric_limits<uint64_t>::max();
    REQUIRE(entry.sizeInBytes() == 0ULL);
}

SECTION("testPartitionRangeByteCountOverflow") {
    PartitionRange range{1, std::numeric_limits<uint64_t>::max()};
    REQUIRE(range.byteCount(512) == 0ULL);
}

}
