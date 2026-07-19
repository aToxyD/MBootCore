#include <catch2/catch_test_macros.hpp>

#include "mbootcore/gpt/PartitionManager.hpp"
#include "mbootcore/gpt/GPTModels.hpp"
#include "mbootcore/gpt/Guid.hpp"
#include "mbootcore/domain/Error.hpp"
#include "../mocks/MockFlashDevice.hpp"
#include <zlib.h>
#include <cstring>
#include <limits>

using namespace mbootcore::gpt;
using mbootcore::ByteBuffer;
using mbootcore::ErrorCode;

namespace {

void writeLE32(ByteBuffer& buf, size_t off, uint32_t v) {
    buf[off] = static_cast<uint8_t>(v);
    buf[off+1] = static_cast<uint8_t>(v >> 8);
    buf[off+2] = static_cast<uint8_t>(v >> 16);
    buf[off+3] = static_cast<uint8_t>(v >> 24);
}
void writeLE16(ByteBuffer& buf, size_t off, uint16_t v) {
    buf[off] = static_cast<uint8_t>(v);
    buf[off+1] = static_cast<uint8_t>(v >> 8);
}
void writeLE64(ByteBuffer& buf, size_t off, uint64_t v) {
    for (int i = 0; i < 8; ++i)
        buf[off+i] = static_cast<uint8_t>(v >> (i * 8));
}

void setupGPT(MockFlashDevice& dev, uint64_t numSectors = 1024) {
    auto& storage = dev.rawStorage();
    storage.assign(numSectors * 512, 0);

    ByteBuffer mbr(512, 0);
    mbr[450] = 0xEE;
    mbr[454] = 1;
    uint32_t sz = numSectors > 0xFFFFFFFF ? 0xFFFFFFFF : static_cast<uint32_t>(numSectors);
    for (int i = 0; i < 4; ++i) mbr[458 + i] = static_cast<uint8_t>(sz >> (i * 8));
    mbr[510] = 0x55; mbr[511] = 0xAA;
    std::copy(mbr.begin(), mbr.end(), storage.begin());

    Guid diskGUID = Guid::fromString("DEADBEEF-0102-0304-0506-0708090A0B0C");
    ByteBuffer hdr(512, 0);
    const char* sig = "EFI PART";
    std::memcpy(hdr.data(), sig, 8);
    writeLE32(hdr, 8, 0x00010000);
    writeLE32(hdr, 12, 92);
    writeLE32(hdr, 16, 0);
    writeLE64(hdr, 24, 1);
    writeLE64(hdr, 32, numSectors - 1);
    writeLE64(hdr, 40, 34);
    writeLE64(hdr, 48, numSectors - 34 - 1);
    writeLE32(hdr, 56, diskGUID.data1);
    writeLE16(hdr, 60, diskGUID.data2);
    writeLE16(hdr, 62, diskGUID.data3);
    for (int i = 0; i < 8; ++i) hdr[64 + i] = diskGUID.data4[i];
    writeLE64(hdr, 72, 2);
    writeLE32(hdr, 80, 128);
    writeLE32(hdr, 84, 128);
    writeLE32(hdr, 88, 0);

    ByteBuffer entries(16384, 0);
    Guid bootType = Guid::fromString("49A4D17F-93A3-45C1-A0DE-F50B2EBE2599");
    Guid bootUnique = Guid::fromString("11111111-2222-3333-4444-555555555555");
    writeLE32(entries, 0, bootType.data1);
    writeLE16(entries, 4, bootType.data2);
    writeLE16(entries, 6, bootType.data3);
    for (int i = 0; i < 8; ++i) entries[8+i] = bootType.data4[i];
    writeLE32(entries, 16, bootUnique.data1);
    writeLE16(entries, 20, bootUnique.data2);
    writeLE16(entries, 22, bootUnique.data3);
    for (int i = 0; i < 8; ++i) entries[24+i] = bootUnique.data4[i];
    writeLE64(entries, 32, 34);
    writeLE64(entries, 40, 100);
    const char* bootName = "boot";
    for (size_t i = 0; i < 4 && bootName[i]; ++i)
        entries[56 + i*2] = bootName[i];

    Guid sysType = Guid::fromString("0FC63DAF-8483-4772-8E79-3D69D8477DE4");
    Guid sysUnique = Guid::fromString("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE");
    writeLE32(entries, 128, sysType.data1);
    writeLE16(entries, 132, sysType.data2);
    writeLE16(entries, 134, sysType.data3);
    for (int i = 0; i < 8; ++i) entries[136+i] = sysType.data4[i];
    writeLE32(entries, 144, sysUnique.data1);
    writeLE16(entries, 148, sysUnique.data2);
    writeLE16(entries, 150, sysUnique.data3);
    for (int i = 0; i < 8; ++i) entries[152+i] = sysUnique.data4[i];
    writeLE64(entries, 160, 101);
    writeLE64(entries, 168, 500);
    const char* sysName = "system";
    for (size_t i = 0; i < 6 && sysName[i]; ++i)
        entries[184 + i*2] = sysName[i];

    uint32_t entriesCRC = ::crc32(0, entries.data(), 16384);
    writeLE32(hdr, 88, entriesCRC);

    ByteBuffer hdrCRCData(hdr.begin(), hdr.begin() + 92);
    uint32_t hdrCRC = ::crc32(0, hdrCRCData.data(), 92);
    writeLE32(hdr, 16, hdrCRC);

    std::copy(hdr.begin(), hdr.end(), storage.begin() + 512);
    std::copy(entries.begin(), entries.end(), storage.begin() + 1024);
    std::copy(entries.begin(), entries.end(), storage.begin() + (numSectors - 33) * 512);

    hdr = ByteBuffer(512, 0);
    std::memcpy(hdr.data(), sig, 8);
    writeLE32(hdr, 8, 0x00010000);
    writeLE32(hdr, 12, 92);
    writeLE32(hdr, 16, 0);
    writeLE64(hdr, 24, numSectors - 1);
    writeLE64(hdr, 32, 1);
    writeLE64(hdr, 40, 34);
    writeLE64(hdr, 48, numSectors - 34 - 1);
    writeLE32(hdr, 56, diskGUID.data1);
    writeLE16(hdr, 60, diskGUID.data2);
    writeLE16(hdr, 62, diskGUID.data3);
    for (int i = 0; i < 8; ++i) hdr[64 + i] = diskGUID.data4[i];
    writeLE64(hdr, 72, numSectors - 33);
    writeLE32(hdr, 80, 128);
    writeLE32(hdr, 84, 128);
    writeLE32(hdr, 88, entriesCRC);

    ByteBuffer bkpCRC(hdr.begin(), hdr.begin() + 92);
    hdrCRC = ::crc32(0, bkpCRC.data(), 92);
    writeLE32(hdr, 16, hdrCRC);
    std::copy(hdr.begin(), hdr.end(), storage.begin() + (numSectors - 1) * 512);
}

}

TEST_CASE("PartitionManagerTest", "[gpt]") {

SECTION("testOpen") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    auto result = mgr.open();
    REQUIRE(result.isOk());
    REQUIRE(mgr.isOpen());
}

SECTION("testRefreshTable") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    auto result = mgr.refreshTable();
    REQUIRE(result.isOk());
    REQUIRE(result.value().usedEntryCount() == 2ULL);
}

SECTION("testListPartitions") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    auto result = mgr.listPartitions();
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 2ULL);
    REQUIRE(result.value()[0].name == "boot");
    REQUIRE(result.value()[1].name == "system");
}

SECTION("testFindByName") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    REQUIRE(mgr.refreshTable().isOk());
    auto boot = mgr.findByName("boot");
    REQUIRE(boot.isOk());
    REQUIRE(boot.value().firstLBA == 34ULL);
    REQUIRE(boot.value().lastLBA == 100ULL);

    auto sys = mgr.findByName("system");
    REQUIRE(sys.isOk());
    REQUIRE(sys.value().firstLBA == 101ULL);

    auto missing = mgr.findByName("nonexistent");
    REQUIRE(missing.isError());
    REQUIRE(missing.error() == ErrorCode::PartitionNotFound);
}

SECTION("testFindByGUID") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    REQUIRE(mgr.refreshTable().isOk());
    auto bootGUID = Guid::fromString("11111111-2222-3333-4444-555555555555");
    auto result = mgr.findByGUID(bootGUID);
    REQUIRE(result.isOk());
    REQUIRE(result.value().name == "boot");

    auto missing = mgr.findByGUID(Guid::Null);
    REQUIRE(missing.isError());
}

SECTION("testFindByType") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    REQUIRE(mgr.refreshTable().isOk());
    auto bootType = Guid::fromString("49A4D17F-93A3-45C1-A0DE-F50B2EBE2599");
    auto result = mgr.findByType(bootType);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 1ULL);

    auto linuxType = Guid::fromString("0FC63DAF-8483-4772-8E79-3D69D8477DE4");
    auto result2 = mgr.findByType(linuxType);
    REQUIRE(result2.isOk());
    REQUIRE(result2.value().size() == 1ULL);
}

SECTION("testExists") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    REQUIRE(mgr.exists("boot"));
    REQUIRE(mgr.exists("system"));
    REQUIRE(!mgr.exists("nonexistent"));
}

SECTION("testPartitionCount") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);
    REQUIRE(mgr.refreshTable().isOk());
    REQUIRE(mgr.partitionCount() == 2ULL);
}

SECTION("testReadPartition") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    auto& storage = dev.rawStorage();
    for (uint64_t i = 34 * 512; i < 101 * 512; ++i) {
        storage[static_cast<size_t>(i)] = static_cast<uint8_t>(i & 0xFF);
    }

    auto result = mgr.readPartition("boot");
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 67 * 512ULL);
    REQUIRE(result.value()[0] == static_cast<uint8_t>((34 * 512) & 0xFF));
}

SECTION("testWritePartition") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    ByteBuffer data(100, 0xAB);
    auto result = mgr.writePartition("boot", data);
    REQUIRE(result.isOk());

    auto readResult = dev.readMemory(34 * 512, 100);
    REQUIRE(readResult.isOk());
    for (size_t i = 0; i < 100; ++i) {
        REQUIRE(readResult.value()[i] == static_cast<uint8_t>(0xAB));
    }
}

SECTION("testErasePartition") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    auto& storage = dev.rawStorage();
    for (size_t i = 34 * 512; i < 101 * 512; ++i)
        storage[i] = 0xFF;

    auto result = mgr.erasePartition("boot");
    REQUIRE(result.isOk());

    auto readResult = dev.readMemory(34 * 512, (101 - 34 + 1) * 512);
    REQUIRE(readResult.isOk());
    for (auto byte : readResult.value()) {
        REQUIRE(byte == 0);
    }
}

SECTION("testBackupRestore") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    ByteBuffer original(100, 0x42);
    auto writeResult = mgr.writePartition("boot", original);
    REQUIRE(writeResult.isOk());

    auto backupResult = mgr.backupPartition("boot");
    REQUIRE(backupResult.isOk());
    REQUIRE(backupResult.value().size() == 67 * 512ULL);

    mgr.erasePartition("boot");

    auto restoreResult = mgr.restorePartition("boot", backupResult.value());
    REQUIRE(restoreResult.isOk());

    auto verifyResult = mgr.verifyPartition("boot", backupResult.value());
    REQUIRE(verifyResult.isOk());
    REQUIRE(verifyResult.value());
}

SECTION("testVerifyPartition") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    auto bootInfo = mgr.findByName("boot");
    REQUIRE(bootInfo.isOk());
    ByteBuffer fullData(static_cast<size_t>(bootInfo.value().byteLength), 0xAA);
    mgr.writePartition("boot", fullData);

    auto result = mgr.verifyPartition("boot", fullData);
    REQUIRE(result.isOk());
    REQUIRE(result.value());

    ByteBuffer wrongData(static_cast<size_t>(bootInfo.value().byteLength), 0xBB);
    auto result2 = mgr.verifyPartition("boot", wrongData);
    REQUIRE(result2.isOk());
    REQUIRE(!result2.value());
}

SECTION("testComparePartition") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    auto bootInfo = mgr.findByName("boot");
    REQUIRE(bootInfo.isOk());
    ByteBuffer fullData(static_cast<size_t>(bootInfo.value().byteLength), 0xCC);
    mgr.writePartition("boot", fullData);

    auto result = mgr.comparePartition("boot", fullData);
    REQUIRE(result.isOk());
    REQUIRE(result.value());
}

SECTION("testTrimPartition") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    auto& storage = dev.rawStorage();
    for (size_t i = 34 * 512; i < 101 * 512; ++i)
        storage[i] = 0xFF;

    auto result = mgr.trimPartition("boot");
    REQUIRE(result.isOk());

    auto readResult = dev.readMemory(34 * 512, (101 - 34 + 1) * 512);
    REQUIRE(readResult.isOk());
    for (auto byte : readResult.value()) {
        REQUIRE(byte == 0);
    }
}

SECTION("testRecoverFromBackup") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    dev.rawStorage()[512] = 0x00;

    auto result = mgr.recoverFromBackup();
    REQUIRE(result.isOk());

    auto table = mgr.refreshTable();
    REQUIRE(table.isOk());
    REQUIRE(table.value().primaryValid);
    REQUIRE(table.value().backupValid);
}

SECTION("testPartitionNotFound") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    PartitionManager mgr(dev);

    auto result = mgr.readPartition("nonexistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PartitionNotFound);

    auto result2 = mgr.writePartition("nonexistent", ByteBuffer(10));
    REQUIRE(result2.isError());
    REQUIRE(result2.error() == ErrorCode::PartitionNotFound);
}

SECTION("testLargeDiskPartitions") {
    MockFlashDevice dev(100000, 512);
    setupGPT(dev, 100000);
    PartitionManager mgr(dev);
    REQUIRE(mgr.refreshTable().isOk());
    auto partitions = mgr.listPartitions();
    REQUIRE(partitions.isOk());
    REQUIRE(partitions.value().size() == 2ULL);

    auto boot = mgr.findByName("boot");
    REQUIRE(boot.isOk());
    REQUIRE(boot.value().byteOffset == 34 * 512ULL);
}

SECTION("testEntryToInfoOverflow") {
    MockFlashDevice dev(1024, 512);
    setupGPT(dev);
    auto& storage = dev.rawStorage();

    // Modify first entry's firstLBA to a value that overflows when * 512
    // UINT64_MAX / 512 ≈ 36e15; any value above that overflows
    writeLE64(storage, 1024 + 32, std::numeric_limits<uint64_t>::max() / 2);
    writeLE64(storage, 1024 + 40, std::numeric_limits<uint64_t>::max() / 2 + 100);

    // Recompute entries CRC
    uint32_t newCRC = ::crc32(0, storage.data() + 1024, 16384);

    // Update primary header
    writeLE32(storage, 512 + 88, newCRC);
    storage[512 + 16] = storage[512 + 17] = storage[512 + 18] = storage[512 + 19] = 0;
    writeLE32(storage, 512 + 16, ::crc32(0, storage.data() + 512, 92));

    // Update backup entries and header
    size_t bkpEntries = static_cast<size_t>(1024 - 33) * 512;
    std::copy(storage.begin() + 1024, storage.begin() + 1024 + 16384,
              storage.begin() + bkpEntries);
    size_t bkpHdr = static_cast<size_t>(1024 - 1) * 512;
    writeLE32(storage, bkpHdr + 88, newCRC);
    storage[bkpHdr + 16] = storage[bkpHdr + 17] = storage[bkpHdr + 18] = storage[bkpHdr + 19] = 0;
    writeLE32(storage, bkpHdr + 16, ::crc32(0, storage.data() + bkpHdr, 92));

    PartitionManager mgr(dev);
    auto result = mgr.listPartitions();
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 2ULL);
    // Overflow entry: firstLBA * sectorSize overflows → byteOffset = 0
    REQUIRE(result.value()[0].byteOffset == 0ULL);
    // Normal entry: firstLBA = 101, sectorSize = 512
    REQUIRE(result.value()[1].byteOffset == 101 * 512ULL);
}

}
