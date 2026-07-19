#include <catch2/catch_test_macros.hpp>

#include "mbootcore/gpt/GPTParser.hpp"
#include "mbootcore/gpt/GPTWriter.hpp"
#include "mbootcore/gpt/GPTModels.hpp"
#include "mbootcore/gpt/Guid.hpp"
#include "../mocks/MockFlashDevice.hpp"
#include <memory>
#include <limits>
#include <zlib.h>

using namespace mbootcore::gpt;
using mbootcore::ByteBuffer;
using mbootcore::ErrorCode;

namespace {

ByteBuffer createProtectiveMBR(uint64_t diskSectors) {
    ByteBuffer mbr(512, 0);
    mbr[446] = 0x00;
    mbr[447] = 0x00; mbr[448] = 0x02; mbr[449] = 0x00;
    mbr[450] = 0xEE;
    mbr[451] = 0xFF; mbr[452] = 0xFF; mbr[453] = 0xFF;
    mbr[454] = 0x01; mbr[455] = 0x00; mbr[456] = 0x00; mbr[457] = 0x00;
    uint32_t size = diskSectors > 0xFFFFFFFF ? 0xFFFFFFFF : static_cast<uint32_t>(diskSectors);
    for (int i = 0; i < 4; ++i) {
        mbr[458 + i] = static_cast<uint8_t>(size >> (i * 8));
    }
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    return mbr;
}

void writeLE16(ByteBuffer& buf, size_t off, uint16_t v) {
    buf[off] = static_cast<uint8_t>(v);
    buf[off+1] = static_cast<uint8_t>(v >> 8);
}
void writeLE32(ByteBuffer& buf, size_t off, uint32_t v) {
    buf[off] = static_cast<uint8_t>(v);
    buf[off+1] = static_cast<uint8_t>(v >> 8);
    buf[off+2] = static_cast<uint8_t>(v >> 16);
    buf[off+3] = static_cast<uint8_t>(v >> 24);
}
void writeLE64(ByteBuffer& buf, size_t off, uint64_t v) {
    for (int i = 0; i < 8; ++i)
        buf[off+i] = static_cast<uint8_t>(v >> (i * 8));
}

ByteBuffer createGPTHeader(uint64_t myLBA, uint64_t altLBA,
                            uint64_t firstUsable, uint64_t lastUsable,
                            uint64_t entryLBA, uint32_t numEntries,
                            uint32_t entrySize,
                            const Guid& diskGUID,
                            uint32_t entryCRC) {
    ByteBuffer hdr(512, 0);
    const char* sig = "EFI PART";
    std::memcpy(hdr.data(), sig, 8);
    writeLE32(hdr, 8, 0x00010000);
    writeLE32(hdr, 12, 92);
    writeLE32(hdr, 16, 0);
    writeLE32(hdr, 20, 0);
    writeLE64(hdr, 24, myLBA);
    writeLE64(hdr, 32, altLBA);
    writeLE64(hdr, 40, firstUsable);
    writeLE64(hdr, 48, lastUsable);
    writeLE32(hdr, 56, diskGUID.data1);
    writeLE16(hdr, 60, diskGUID.data2);
    writeLE16(hdr, 62, diskGUID.data3);
    for (int i = 0; i < 8; ++i)
        hdr[64 + i] = diskGUID.data4[i];
    writeLE64(hdr, 72, entryLBA);
    writeLE32(hdr, 80, numEntries);
    writeLE32(hdr, 84, entrySize);
    writeLE32(hdr, 88, entryCRC);

    ByteBuffer crcData(hdr.begin(), hdr.begin() + 92);
    uint32_t crc = ::crc32(0, crcData.data(), 92);
    writeLE32(hdr, 16, crc);
    return hdr;
}

void writePartitionEntry(ByteBuffer& buf, size_t off,
                          const Guid& typeGUID, const Guid& uniqueGUID,
                          uint64_t firstLBA, uint64_t lastLBA,
                          uint64_t attrs, const char16_t* name) {
    if (off + 128 > buf.size()) buf.resize(off + 128, 0);
    writeLE32(buf, off, typeGUID.data1);
    writeLE16(buf, off+4, typeGUID.data2);
    writeLE16(buf, off+6, typeGUID.data3);
    for (int i = 0; i < 8; ++i)
        buf[off+8+i] = typeGUID.data4[i];

    writeLE32(buf, off+16, uniqueGUID.data1);
    writeLE16(buf, off+20, uniqueGUID.data2);
    writeLE16(buf, off+22, uniqueGUID.data3);
    for (int i = 0; i < 8; ++i)
        buf[off+24+i] = uniqueGUID.data4[i];

    writeLE64(buf, off+32, firstLBA);
    writeLE64(buf, off+40, lastLBA);
    writeLE64(buf, off+48, attrs);

    for (int i = 0; i < 36 && name[i] != 0; ++i) {
        writeLE16(buf, off+56+i*2, static_cast<uint16_t>(name[i]));
    }
}

uint32_t computeEntryCRC(const ByteBuffer& entries, uint64_t count, uint64_t entrySize) {
    size_t total = static_cast<size_t>(count * entrySize);
    if (entries.size() < total) total = entries.size();
    return ::crc32(0, entries.data(), static_cast<uInt>(total));
}

std::unique_ptr<MockFlashDevice> createValidGPTDevice(uint64_t numSectors = 1024) {
    auto dev = std::make_unique<MockFlashDevice>(numSectors, 512);
    auto& storage = dev->rawStorage();
    storage.assign(numSectors * 512, 0);

    auto mbr = createProtectiveMBR(numSectors);
    std::copy(mbr.begin(), mbr.end(), storage.begin());

    ByteBuffer entries(16384, 0);

    Guid bootGUID = Guid::fromString("49A4D17F-93A3-45C1-A0DE-F50B2EBE2599");
    Guid bootUnique = Guid::fromString("11111111-2222-3333-4444-555555555555");
    char16_t bootName[] = u"boot";
    writePartitionEntry(entries, 0, bootGUID, bootUnique, 34, 100, 0, bootName);

    Guid sysGUID = Guid::fromString("0FC63DAF-8483-4772-8E79-3D69D8477DE4");
    Guid sysUnique = Guid::fromString("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE");
    char16_t sysName[] = u"system";
    writePartitionEntry(entries, 128, sysGUID, sysUnique, 101, 500, 0, sysName);

    uint32_t entriesCRC = computeEntryCRC(entries, 128, 128);

    Guid diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    auto primaryHdr = createGPTHeader(1, numSectors - 1, 34, numSectors - 34 - 1,
                                       2, 128, 128, diskGUID, entriesCRC);
    std::copy(primaryHdr.begin(), primaryHdr.end(), storage.begin() + 512);

    std::copy(entries.begin(), entries.end(), storage.begin() + 2 * 512);

    std::copy(entries.begin(), entries.end(), storage.begin() + (numSectors - 33) * 512);

    auto backupHdr = createGPTHeader(numSectors - 1, 1, 34, numSectors - 34 - 1,
                                      numSectors - 33, 128, 128, diskGUID, entriesCRC);
    std::copy(backupHdr.begin(), backupHdr.end(), storage.begin() + (numSectors - 1) * 512);

    return dev;
}

}

TEST_CASE("GPTParserTest", "[gpt]") {

SECTION("testValidGPT") {
    auto dev = createValidGPTDevice();
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.hasValidProtectiveMBR);
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.backupValid);
    REQUIRE(result.table.entriesValid);
    REQUIRE(result.table.entryCount == 128ULL);
    REQUIRE(result.table.usedEntryCount() == 2ULL);
    REQUIRE(result.errors.empty());
    REQUIRE(result.warnings.size() == 0ULL);

    REQUIRE(result.table.primaryHeader.signature == GPTHeader::Signature);
    REQUIRE(result.table.primaryHeader.myLBA == 1ULL);
    REQUIRE(result.table.primaryHeader.alternateLBA == 1023ULL);
    REQUIRE(result.table.primaryHeader.firstUsableLBA == 34ULL);
    REQUIRE(result.table.primaryHeader.lastUsableLBA == 989ULL);
    REQUIRE(result.table.primaryHeader.numberOfPartitionEntries == 128u);
    REQUIRE(result.table.primaryHeader.sizeOfPartitionEntry == 128u);

    REQUIRE(result.table.entries[0].firstLBA == 34ULL);
    REQUIRE(result.table.entries[0].lastLBA == 100ULL);
    REQUIRE(result.table.entries[1].firstLBA == 101ULL);
    REQUIRE(result.table.entries[1].lastLBA == 500ULL);

    REQUIRE(parser.hasValidProtectiveMBR());
}

SECTION("testValidBackupGPT") {
    auto dev = createValidGPTDevice(2048);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.backupValid);
    REQUIRE(result.table.entriesValid);
    REQUIRE(result.table.usedEntryCount() == 2ULL);
}

SECTION("testCorruptedPrimary") {
    auto dev = createValidGPTDevice();
    dev->rawStorage()[512] = 0x00;
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    REQUIRE(result.table.backupValid);
    REQUIRE(result.warnings.size() == 1ULL);
}

SECTION("testCorruptedBackup") {
    auto dev = createValidGPTDevice();
    size_t backupOff = (1024 - 1) * 512;
    dev->rawStorage()[backupOff] = 0x00;
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.primaryValid);
    REQUIRE(!result.table.backupValid);
    REQUIRE(result.warnings.size() == 1ULL);
}

SECTION("testBadCRC") {
    auto dev = createValidGPTDevice();
    dev->rawStorage()[512 + 24] = 0xFF;
    dev->rawStorage()[512 + 16] ^= 0xFF;
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    REQUIRE(result.table.backupValid);
}

SECTION("testDuplicatePartition") {
    auto dev = createValidGPTDevice();
    auto& storage = dev->rawStorage();
    Guid dupGUID = Guid::fromString("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7");
    Guid dupUnique = Guid::fromString("FFFFFFFF-EEEE-DDDD-CCCC-BBBBBBBBBBBB");
    char16_t dupName[] = u"boot";
    ByteBuffer oldEntries(16384, 0);
    std::copy(storage.begin() + 1024, storage.begin() + 1024 + 16384, oldEntries.begin());
    writePartitionEntry(oldEntries, 128 * 2, dupGUID, dupUnique, 600, 700, 0, dupName);
    uint32_t newCRC = computeEntryCRC(oldEntries, 128, 128);
    std::copy(oldEntries.begin(), oldEntries.end(), storage.begin() + 1024);
    std::copy(oldEntries.begin(), oldEntries.end(), storage.begin() + (1024 - 33) * 512);

    writeLE32(storage, 512 + 88, newCRC);
    ByteBuffer hdr(storage.begin() + 512, storage.begin() + 512 + 92);
    hdr[16] = hdr[17] = hdr[18] = hdr[19] = 0;
    uint32_t hdrCRC = ::crc32(0, hdr.data(), 92);
    writeLE32(storage, 512 + 16, hdrCRC);
    size_t backupOff = (1024 - 1) * 512;
    std::copy(storage.begin() + 512, storage.begin() + 1024, storage.begin() + backupOff);

    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.usedEntryCount() == 3ULL);
}

SECTION("testInvalidGUID") {
    auto dev = createValidGPTDevice();
    auto& storage = dev->rawStorage();
    Guid badGUID = Guid::fromString("00000000-0000-0000-0000-000000000000");
    writePartitionEntry(storage, 1024 + 128,
                         badGUID, badGUID, 0, 0, 0, u"");

    ByteBuffer entries(16384, 0);
    std::copy(storage.begin() + 1024, storage.begin() + 1024 + 16384, entries.begin());
    uint32_t newCRC = computeEntryCRC(entries, 128, 128);
    writeLE32(storage, 512 + 88, newCRC);
    ByteBuffer hdr(storage.begin() + 512, storage.begin() + 512 + 92);
    hdr[16] = hdr[17] = hdr[18] = hdr[19] = 0;
    uint32_t hdrCRC = ::crc32(0, hdr.data(), 92);
    writeLE32(storage, 512 + 16, hdrCRC);

    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.usedEntryCount() == 1ULL);
}

SECTION("testUTF16Names") {
    auto dev = createValidGPTDevice();
    auto& storage = dev->rawStorage();

    char16_t utf16Name[] = u"system_b";
    ByteBuffer entries(16384, 0);
    std::copy(storage.begin() + 1024, storage.begin() + 1024 + 16384, entries.begin());
    writePartitionEntry(entries, 128, PartitionTypes::LinuxFS,
                         Guid::fromString("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"),
                         101, 500, 0, utf16Name);
    uint32_t newCRC = computeEntryCRC(entries, 128, 128);
    std::copy(entries.begin(), entries.end(), storage.begin() + 1024);

    writeLE32(storage, 512 + 88, newCRC);
    ByteBuffer hdr(storage.begin() + 512, storage.begin() + 512 + 92);
    hdr[16] = hdr[17] = hdr[18] = hdr[19] = 0;
    uint32_t hdrCRC = ::crc32(0, hdr.data(), 92);
    writeLE32(storage, 512 + 16, hdrCRC);

    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.entries[1].nameToString() == "system_b");
}

SECTION("testLookupByName") {
    auto dev = createValidGPTDevice();
    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);

    bool foundBoot = false;
    bool foundSystem = false;
    for (const auto& entry : result.table.entries) {
        if (entry.isUsed()) {
            std::string name = entry.nameToString();
            if (name == "boot") foundBoot = true;
            if (name == "system") foundSystem = true;
        }
    }
    REQUIRE(foundBoot);
    REQUIRE(foundSystem);
}

SECTION("testLookupByGUID") {
    auto dev = createValidGPTDevice();
    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);

    auto bootGUID = Guid::fromString("49A4D17F-93A3-45C1-A0DE-F50B2EBE2599");
    auto sysGUID = Guid::fromString("0FC63DAF-8483-4772-8E79-3D69D8477DE4");

    bool foundBoot = false;
    bool foundSystem = false;
    for (const auto& entry : result.table.entries) {
        if (entry.partitionTypeGUID == bootGUID) foundBoot = true;
        if (entry.partitionTypeGUID == sysGUID) foundSystem = true;
    }
    REQUIRE(foundBoot);
    REQUIRE(foundSystem);
}

SECTION("testLargeGPT") {
    auto dev = createValidGPTDevice(100000);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.backupValid);
    REQUIRE(result.table.primaryHeader.alternateLBA == 99999ULL);
    REQUIRE(result.table.primaryHeader.lastUsableLBA == 99965ULL);
    REQUIRE(result.table.diskSizeInSectors == 100000ULL);
}

SECTION("testEmptyGPT") {
    auto dev = createValidGPTDevice();
    auto& storage = dev->rawStorage();

    ByteBuffer emptyEntries(16384, 0);
    uint32_t emptyCRC = computeEntryCRC(emptyEntries, 128, 128);
    std::copy(emptyEntries.begin(), emptyEntries.end(), storage.begin() + 1024);
    writeLE32(storage, 512 + 88, emptyCRC);
    ByteBuffer hdr(storage.begin() + 512, storage.begin() + 512 + 92);
    hdr[16] = hdr[17] = hdr[18] = hdr[19] = 0;
    uint32_t hdrCRC = ::crc32(0, hdr.data(), 92);
    writeLE32(storage, 512 + 16, hdrCRC);

    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.usedEntryCount() == 0ULL);
}

SECTION("testMissingGPT") {
    MockFlashDevice dev(1024, 512);
    GPTParser parser(dev);
    auto result = parser.parse();

    REQUIRE(!result.table.hasValidProtectiveMBR);
    REQUIRE(!result.table.primaryValid);
    REQUIRE(!result.table.backupValid);
    REQUIRE(!result.table.entriesValid);
}

SECTION("testNullEntries") {
    auto dev = createValidGPTDevice();
    auto& storage = dev->rawStorage();

    std::fill(storage.begin() + 1024, storage.begin() + 1024 + 16, 0);
    ByteBuffer entries(16384, 0);
    std::copy(storage.begin() + 1024, storage.begin() + 1024 + 16384, entries.begin());
    uint32_t newCRC = computeEntryCRC(entries, 128, 128);
    writeLE32(storage, 512 + 88, newCRC);
    ByteBuffer hdr(storage.begin() + 512, storage.begin() + 512 + 92);
    hdr[16] = hdr[17] = hdr[18] = hdr[19] = 0;
    uint32_t hdrCRC = ::crc32(0, hdr.data(), 92);
    writeLE32(storage, 512 + 16, hdrCRC);

    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);
    REQUIRE(!result.table.entries[0].isUsed());
    REQUIRE(result.table.usedEntryCount() == 1ULL);
}

SECTION("testEntryOverlap") {
    auto dev = createValidGPTDevice();
    auto& storage = dev->rawStorage();
    Guid extraGUID = PartitionTypes::BasicData;
    Guid extraUnique = Guid::fromString("CCCCCCCC-DDDD-EEEE-FFFF-111111111111");
    ByteBuffer entries(16384, 0);
    std::copy(storage.begin() + 1024, storage.begin() + 1024 + 16384, entries.begin());
    writePartitionEntry(entries, 128 * 2, extraGUID, extraUnique, 400, 600, 0, u"extra");
    uint32_t newCRC = computeEntryCRC(entries, 128, 128);
    std::copy(entries.begin(), entries.end(), storage.begin() + 1024);
    writeLE32(storage, 512 + 88, newCRC);
    ByteBuffer hdr(storage.begin() + 512, storage.begin() + 512 + 92);
    hdr[16] = hdr[17] = hdr[18] = hdr[19] = 0;
    uint32_t hdrCRC = ::crc32(0, hdr.data(), 92);
    writeLE32(storage, 512 + 16, hdrCRC);

    GPTParser parser(*dev);
    auto result = parser.parse();
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.usedEntryCount() == 3ULL);
}

}

// Helper: create a device with a GPT header containing specified entry fields.
// The entry CRC is computed over zero-filled entries to match.
std::unique_ptr<MockFlashDevice> createDeviceWithHeaderFields(
        uint64_t numSectors, uint32_t numEntries, uint32_t entrySize) {
    auto dev = std::make_unique<MockFlashDevice>(numSectors, 512);
    auto& storage = dev->rawStorage();
    storage.assign(static_cast<size_t>(numSectors) * 512, 0);

    auto mbr = createProtectiveMBR(numSectors);
    std::copy(mbr.begin(), mbr.end(), storage.begin());

    uint32_t entriesCRC = 0;
    bool canAllocateEntries = numEntries <= 4096 && entrySize <= 512 &&
                              static_cast<uint64_t>(numEntries) * entrySize <= 4096 * 512;

    ByteBuffer entries;
    if (canAllocateEntries) {
        entries.assign(static_cast<size_t>(numEntries) * entrySize, 0);
        entriesCRC = ::crc32(0, entries.data(), static_cast<uInt>(entries.size()));
    } else {
        entries.assign(256, 0);
        entriesCRC = ::crc32(0, entries.data(), 256);
    }

    Guid diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    auto hdr = createGPTHeader(1, numSectors - 1, 34, numSectors - 34 - 1,
                               2, numEntries, entrySize, diskGUID, entriesCRC);
    std::copy(hdr.begin(), hdr.end(), storage.begin() + 512);

    if (canAllocateEntries) {
        size_t entryBytes = static_cast<size_t>(numEntries) * entrySize;
        size_t backupLBA = (numSectors > 33) ? static_cast<size_t>(numSectors - 33) : 0;
        if (numSectors >= 35 && entryBytes <= storage.size() - 2 * 512) {
            std::copy(entries.begin(), entries.end(), storage.begin() + 2 * 512);
        }
        if (backupLBA > 0 && entryBytes <= storage.size() - backupLBA * 512) {
            std::copy(entries.begin(), entries.end(),
                      storage.begin() + backupLBA * 512);
        }
    }

    auto backupHdr = createGPTHeader(numSectors - 1, 1, 34, numSectors - 34 - 1,
                                     numSectors - 33, numEntries, entrySize,
                                     diskGUID, entriesCRC);
    size_t backupOffset = static_cast<size_t>(numSectors - 1) * 512;
    if (backupOffset + 512 <= storage.size()) {
        std::copy(backupHdr.begin(), backupHdr.end(),
                  storage.begin() + backupOffset);
    }

    return dev;
}

// Helper: create a device with a GPT header whose partitionEntryLBA
// is set to the specified value. Used to test LBA→byte overflow.
std::unique_ptr<MockFlashDevice> createDeviceWithEntryLBA(
        uint64_t entryLBA, uint64_t numSectors = 1024) {
    constexpr uint32_t kSectorSize = 512;
    auto dev = std::make_unique<MockFlashDevice>(numSectors, kSectorSize);
    auto& storage = dev->rawStorage();
    storage.assign(static_cast<size_t>(numSectors) * kSectorSize, 0);

    auto mbr = createProtectiveMBR(numSectors);
    std::copy(mbr.begin(), mbr.end(), storage.begin());

    ByteBuffer entries(16384, 0);
    uint32_t entriesCRC = ::crc32(0, entries.data(),
                                   static_cast<uInt>(entries.size()));

    Guid diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    auto hdr = createGPTHeader(1, numSectors - 1, 34, numSectors - 34 - 1,
                               entryLBA, 128, 128, diskGUID, entriesCRC);
    std::copy(hdr.begin(), hdr.end(), storage.begin() + kSectorSize);

    return dev;
}

TEST_CASE("GPTParserHardeningTest", "[gpt]") {

SECTION("testCase1_ZeroPartitionEntries") {
    // numberOfPartitionEntries = 0
    auto dev = createDeviceWithHeaderFields(1024, 0, 128);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
    bool foundInvalidHeader = false;
    for (auto w : result.warnings) {
        if (w == ErrorCode::GPTInvalidHeader) foundInvalidHeader = true;
    }
    REQUIRE(foundInvalidHeader);
}

SECTION("testCase2_MaxUInt32PartitionEntries") {
    // numberOfPartitionEntries = 0xFFFFFFFF
    auto dev = createDeviceWithHeaderFields(1024, 0xFFFFFFFF, 128);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
    bool foundInvalidHeader = false;
    for (auto w : result.warnings) {
        if (w == ErrorCode::GPTInvalidHeader) foundInvalidHeader = true;
    }
    REQUIRE(foundInvalidHeader);
}

SECTION("testCase3_EntrySizeTooSmall") {
    // sizeOfPartitionEntry = 64 (below minimum 128)
    auto dev = createDeviceWithHeaderFields(1024, 128, 64);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
    bool foundInvalidHeader = false;
    for (auto w : result.warnings) {
        if (w == ErrorCode::GPTInvalidHeader) foundInvalidHeader = true;
    }
    REQUIRE(foundInvalidHeader);
}

SECTION("testCase4_EntrySizeTooLarge") {
    // sizeOfPartitionEntry = 4096 (above maximum 512)
    auto dev = createDeviceWithHeaderFields(1024, 128, 4096);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
    bool foundInvalidHeader = false;
    for (auto w : result.warnings) {
        if (w == ErrorCode::GPTInvalidHeader) foundInvalidHeader = true;
    }
    REQUIRE(foundInvalidHeader);
}

SECTION("testCase5_EntryCountExceedsMax") {
    // entryCount exceeds kMaxPartitionEntries (4097 > 4096)
    auto dev = createDeviceWithHeaderFields(1024, 4097, 128);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(!result.table.primaryValid);
    bool foundInvalidHeader = false;
    for (auto w : result.warnings) {
        if (w == ErrorCode::GPTInvalidHeader) foundInvalidHeader = true;
    }
    REQUIRE(foundInvalidHeader);
}

SECTION("testCase6_ValidGPTStillParses") {
    // Valid GPT must still parse successfully after hardening
    auto dev = createValidGPTDevice();
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.hasValidProtectiveMBR);
    REQUIRE(result.table.primaryValid);
    REQUIRE(result.table.backupValid);
    REQUIRE(result.table.entriesValid);
    REQUIRE(result.table.entryCount == 128ULL);
    REQUIRE(result.table.usedEntryCount() == 2ULL);
    REQUIRE(result.errors.empty());
    REQUIRE(result.warnings.empty());
}

SECTION("testCase7_LbaOverflow_partitionEntryLBA") {
    // partitionEntryLBA chosen so that lba * 512 overflows to 0.
    constexpr uint64_t kOverflowLBA = 0x8000000000000ULL;
    auto dev = createDeviceWithEntryLBA(kOverflowLBA);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
    bool hasIssue = !result.errors.empty() || !result.warnings.empty();
    REQUIRE(hasIssue);
}

SECTION("testCase8_LbaOverflow_maxUint64") {
    auto dev = createDeviceWithEntryLBA(std::numeric_limits<uint64_t>::max());
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
    bool hasIssue = !result.errors.empty() || !result.warnings.empty();
    REQUIRE(hasIssue);
}

SECTION("testCase9_MaxValidLba_noOverflow") {
    // Largest LBA where lba * 512 does NOT overflow.
    // The read fails because the device is too small, not because of overflow.
    constexpr uint64_t kSectorSize = 512;
    constexpr uint64_t kMaxValidLBA = std::numeric_limits<uint64_t>::max() / kSectorSize;
    auto dev = createDeviceWithEntryLBA(kMaxValidLBA);
    GPTParser parser(*dev);
    auto result = parser.parse();

    REQUIRE(result.table.primaryValid);
    REQUIRE(!result.table.entriesValid);
}

}
