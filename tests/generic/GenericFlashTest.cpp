#include <catch2/catch_test_macros.hpp>

#include <mbootcore/MBootCore.hpp>
#include "mbootcore/generic/adapter/FirehoseAdapter.hpp"
#include "mbootcore/generic/adapter/SaharaAdapter.hpp"
#include "../mocks/MockTransport.hpp"
#include "../mocks/MockLogger.hpp"
#include "../virtual/VirtualFlashDevice.hpp"
#include "../virtual/VirtualFirehoseDevice.hpp"
#include "../virtual/VirtualSaharaDevice.hpp"

#include <limits>

using namespace mbootcore;

TEST_CASE("GenericFlashTest", "[generic]") {

SECTION("testCapabilities") {
    FlashCapability all = FlashCapability::Read | FlashCapability::Write | FlashCapability::Erase;
    REQUIRE(hasCapability(all, FlashCapability::Read));
    REQUIRE(hasCapability(all, FlashCapability::Write));
    REQUIRE(hasCapability(all, FlashCapability::Erase));
    REQUIRE(!hasCapability(all, FlashCapability::Reset));

    FlashCapability none = FlashCapability::None;
    REQUIRE(!hasCapability(none, FlashCapability::Read));

    REQUIRE(toString(FlashCapability::Read) == "Read");
    REQUIRE(toString(FlashCapability::Write) == "Write");
}

SECTION("testDeviceLifecycle") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);

    REQUIRE(!device.isOpen());

    auto result = device.open();
    REQUIRE(result.isOk());
    REQUIRE(device.isOpen());

    device.close();
    REQUIRE(!device.isOpen());
}

SECTION("testSaharaAdapter") {
    MockTransport transport;
    MockLogger logger;

    SaharaAdapter adapter(transport, logger);

    auto caps = adapter.capabilities();
    REQUIRE(hasCapability(caps, FlashCapability::UploadLoader));
    REQUIRE(hasCapability(caps, FlashCapability::Reset));
    REQUIRE(!hasCapability(caps, FlashCapability::Read));

    auto result = adapter.open();
    REQUIRE(!result.isOk());
    REQUIRE(!adapter.isOpen());
}

SECTION("testFirehoseAdapter") {
    MockTransport transport;
    MockLogger logger;
    FirehoseAdapter adapter(transport, logger);

    auto caps = adapter.capabilities();
    REQUIRE(hasCapability(caps, FlashCapability::Read));
    REQUIRE(hasCapability(caps, FlashCapability::Write));
    REQUIRE(hasCapability(caps, FlashCapability::Erase));
    REQUIRE(hasCapability(caps, FlashCapability::Reset));
    REQUIRE(hasCapability(caps, FlashCapability::StorageInfo));

    auto result = adapter.open();
    REQUIRE(!result.isOk());
}

SECTION("testFlashDeviceInfo") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);

    REQUIRE(device.open().isOk());

    auto result = device.deviceInfo();
    REQUIRE(result.isOk());
    REQUIRE(result.value().protocolName == "Virtual");
    REQUIRE(result.value().bootMode == BootMode::EDL);
}

SECTION("testUploadLoader") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    ByteBuffer data(64, 0xBE);
    auto result = device.uploadLoader(data);
    REQUIRE(result.isOk());
}

SECTION("testResetTransition") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    auto result = device.reset();
    REQUIRE(result.isOk());
}

SECTION("testReadPartition") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());
    REQUIRE(device.reset().isOk());

    auto result = device.readPartition("system");
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == 256);
}

SECTION("testWritePartition") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());
    REQUIRE(device.reset().isOk());

    ByteBuffer data(128, 0xFF);
    auto result = device.writePartition("data", data);
    REQUIRE(result.isOk());
}

SECTION("testErasePartition") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());
    REQUIRE(device.reset().isOk());

    auto result = device.erasePartition("cache");
    REQUIRE(result.isOk());
}

SECTION("testReadMemory") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    auto result = device.readMemory(0x1000, 64);
    REQUIRE(!result.isOk());
}

SECTION("testWriteMemory") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    ByteBuffer data(32);
    auto result = device.writeMemory(0x1000, data);
    REQUIRE(!result.isOk());
}

SECTION("testEraseMemory") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    auto result = device.eraseMemory(0x1000, 4096);
    REQUIRE(!result.isOk());
}

SECTION("testGetStorageInfo") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());
    REQUIRE(device.reset().isOk());

    auto result = device.getStorageInfo();
    REQUIRE(result.isOk());
    REQUIRE(result.value().type == StorageType::UFS);
    REQUIRE(result.value().sectorSize == 4096);
}

SECTION("testGetPartitions") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    auto result = device.getPartitions();
    REQUIRE(result.isOk());
    REQUIRE(result.value().entryCount == 2);
}

SECTION("testCancel") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    device.cancel();
    REQUIRE(true);
}

SECTION("testProgressCallback") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    int progressCalls = 0;
    device.setProgressCallback([&](const ProgressInfo& pi) {
        progressCalls++;
        REQUIRE(pi.totalBytes > 0);
    });

    ByteBuffer data(64, 0xBE);
    REQUIRE(device.uploadLoader(data).isOk());
    REQUIRE(progressCalls > 0);
}

SECTION("testNotSupported") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    auto storageResult = device.getStorageInfo();
    REQUIRE(!storageResult.isOk());
    REQUIRE(storageResult.error() == ErrorCode::NotSupported);
}

SECTION("testOperationPipeline") {
    MockTransport transport;
    MockLogger logger;
    VirtualFlashDevice device(transport, logger);
    REQUIRE(device.open().isOk());

    OperationPipeline pipeline(device);

    struct TestOp : IFlashOperation {
        std::string_view name() const noexcept override { return "TestOp"; }
        FlashCapability requiredCapabilities() const noexcept override {
            return FlashCapability::None;
        }
        Result<void> validate() override { return {}; }
        Result<void> execute(ProgressCallback cb) override {
            if (cb) {
                ProgressInfo pi;
                pi.totalBytes = 100;
                pi.transferredBytes = 100;
                pi.percentage = 100.0;
                cb(pi);
SECTION("firehose_open_transportWriteFailure")
{
    MockTransport transport;
    MockLogger logger;
    transport.setWriteResult(ErrorCode::TransportError);
    FirehoseAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    auto result = adapter.open();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::TransportError);
    REQUIRE_FALSE(adapter.isOpen());
}

SECTION("firehose_open_transportReadFailure")
{
    MockTransport transport;
    MockLogger logger;
    transport.setReadResult(ErrorCode::TransportError);
    FirehoseAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    auto result = adapter.open();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::TransportError);
    REQUIRE_FALSE(adapter.isOpen());
}

SECTION("sahara_open_transportReadFailure")
{
    MockTransport transport;
    MockLogger logger;
    transport.setReadResult(ErrorCode::TransportError);
    SaharaAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    auto result = adapter.open();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::TransportError);
    REQUIRE_FALSE(adapter.isOpen());
}

}
            return {};
        }
        void cancel() noexcept override {}
        bool isCancelled() const noexcept override { return false; }
        const ProgressInfo& progress() const noexcept override {
            static ProgressInfo p;
            return p;
        }
    };

    TestOp op;
    auto result = pipeline.execute(op);
    REQUIRE(result.isOk());
}

SECTION("testCapabilityChecking") {
    auto all = FlashCapability::All;
    REQUIRE(hasCapability(all, FlashCapability::Read));
    REQUIRE(hasCapability(all, FlashCapability::Write));
    REQUIRE(hasCapability(all, FlashCapability::Erase));

    auto none = FlashCapability::None;
    REQUIRE(!hasCapability(none, FlashCapability::Read));
}

SECTION("firehose_readMemory_endByteOverflow") {
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE(adapter.open().isOk());

    // byteAddress + byteSize overflows uint64
    constexpr uint64_t kNearMax = std::numeric_limits<uint64_t>::max() - 10;
    auto result = adapter.readMemory(kNearMax, 20);
    REQUIRE(result.isError());
}

SECTION("firehose_writeMemory_endByteOverflow") {
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE(adapter.open().isOk());

    constexpr uint64_t kNearMax = std::numeric_limits<uint64_t>::max() - 10;
    ByteBuffer data(20, 0xAB);
    auto result = adapter.writeMemory(kNearMax, data);
    REQUIRE(result.isError());
}

SECTION("firehose_eraseMemory_endByteOverflow") {
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE(adapter.open().isOk());

    constexpr uint64_t kNearMax = std::numeric_limits<uint64_t>::max() - 10;
    auto result = adapter.eraseMemory(kNearMax, 20);
    REQUIRE(result.isError());
}

SECTION("firehose_readMemory_truncationOverflow") {
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE(adapter.open().isOk());

    // Address large enough that startSector > UINT32_MAX
    constexpr uint64_t kHugeAddr = uint64_t{std::numeric_limits<uint32_t>::max()} * 4096 + 4096;
    auto result = adapter.readMemory(kHugeAddr, 100);
    REQUIRE(result.isError());
}

SECTION("firehose_writeMemory_truncationOverflow") {
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE(adapter.open().isOk());

    constexpr uint64_t kHugeAddr = uint64_t{std::numeric_limits<uint32_t>::max()} * 4096 + 4096;
    ByteBuffer data(100, 0xCD);
    auto result = adapter.writeMemory(kHugeAddr, data);
    REQUIRE(result.isError());
}

SECTION("firehose_eraseMemory_truncationOverflow") {
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE(adapter.open().isOk());

    constexpr uint64_t kHugeAddr = uint64_t{std::numeric_limits<uint32_t>::max()} * 4096 + 4096;
    auto result = adapter.eraseMemory(kHugeAddr, 100);
    REQUIRE(result.isError());
}

SECTION("capacityBytes_overflow") {
    StorageInfo si;
    si.numSectors = std::numeric_limits<uint64_t>::max();
    si.sectorSize = 4096;
    si.capacity = 0;
    REQUIRE(si.capacityBytes() == 0);
}

SECTION("capacityBytes_noOverflow") {
    StorageInfo si;
    si.numSectors = 1000;
    si.sectorSize = 4096;
    si.capacity = 0;
    REQUIRE(si.capacityBytes() == 1000 * 4096);
}

SECTION("capacityBytes_explicitCapacityTakesPrecedence") {
    StorageInfo si;
    si.numSectors = std::numeric_limits<uint64_t>::max();
    si.sectorSize = 4096;
    si.capacity = 2000;
    REQUIRE(si.capacityBytes() == 2000);
}

SECTION("firehose_deviceInfo_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    FirehoseAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    auto result = adapter.deviceInfo();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("firehose_readMemory_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    FirehoseAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    auto result = adapter.readMemory(0x1000, 64);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("firehose_writeMemory_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    FirehoseAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    ByteBuffer data(32, 0xAB);
    auto result = adapter.writeMemory(0x1000, data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("firehose_reset_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    FirehoseAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    auto result = adapter.reset();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("sahara_deviceInfo_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    SaharaAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    auto result = adapter.deviceInfo();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("sahara_uploadLoader_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    SaharaAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    ByteBuffer data(64, 0xBE);
    auto result = adapter.uploadLoader(data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("sahara_reset_notOpen")
{
    MockTransport transport;
    MockLogger logger;
    SaharaAdapter adapter(transport, logger);

    REQUIRE_FALSE(adapter.isOpen());
    auto result = adapter.reset();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::DeviceDisconnected);
}

SECTION("sahara_notSupported_stubs")
{
    MockTransport transport;
    MockLogger logger;
    SaharaAdapter adapter(transport, logger);

    auto r1 = adapter.getStorageInfo();
    REQUIRE(r1.isError());
    REQUIRE(r1.error() == ErrorCode::NotSupported);

    auto r2 = adapter.getPartitions();
    REQUIRE(r2.isError());
    REQUIRE(r2.error() == ErrorCode::NotSupported);

    auto r3 = adapter.readMemory(0, 0);
    REQUIRE(r3.isError());
    REQUIRE(r3.error() == ErrorCode::NotSupported);

    auto r4 = adapter.writeMemory(0, {});
    REQUIRE(r4.isError());
    REQUIRE(r4.error() == ErrorCode::NotSupported);

    auto r5 = adapter.eraseMemory(0, 0);
    REQUIRE(r5.isError());
    REQUIRE(r5.error() == ErrorCode::NotSupported);

    auto r6 = adapter.readPartition("x");
    REQUIRE(r6.isError());
    REQUIRE(r6.error() == ErrorCode::NotSupported);

    auto r7 = adapter.writePartition("x", {});
    REQUIRE(r7.isError());
    REQUIRE(r7.error() == ErrorCode::NotSupported);

    auto r8 = adapter.erasePartition("x");
    REQUIRE(r8.isError());
    REQUIRE(r8.error() == ErrorCode::NotSupported);

    auto r9 = adapter.powerReset();
    REQUIRE(r9.isError());
    REQUIRE(r9.error() == ErrorCode::NotSupported);
}

SECTION("firehose_open_success")
{
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    REQUIRE(adapter.open().isOk());
    REQUIRE(adapter.isOpen());

    auto info = adapter.deviceInfo();
    REQUIRE(info.isOk());
    REQUIRE(info.value().protocolName == "Firehose");
    REQUIRE(info.value().bootMode == BootMode::Firehose);
}

SECTION("firehose_reset_success")
{
    MockTransport transport;
    MockLogger logger;
    VirtualFirehoseDevice dev(transport);
    dev.queueConfigureAck(ConfigureCommand{});

    FirehoseAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    REQUIRE(adapter.open().isOk());
    REQUIRE(adapter.isOpen());

    auto info = adapter.deviceInfo();
    REQUIRE(info.isOk());
    REQUIRE(info.value().protocolName == "Firehose");
    REQUIRE(info.value().bootMode == BootMode::Firehose);

    dev.queueAck("reset");

    auto result = adapter.reset();
    REQUIRE(result.isOk());
    REQUIRE_FALSE(adapter.isOpen());

    auto guardedInfo = adapter.deviceInfo();
    REQUIRE(guardedInfo.isError());
    REQUIRE(guardedInfo.error() == ErrorCode::DeviceDisconnected);
}

SECTION("sahara_open_success")
{
    MockTransport transport;
    MockLogger logger;
    VirtualSaharaDevice::Behavior beh;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();

    SaharaAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    REQUIRE(adapter.open().isOk());
    REQUIRE(adapter.isOpen());

    auto info = adapter.deviceInfo();
    REQUIRE(info.isOk());
    REQUIRE(info.value().protocolName == "Sahara");
    REQUIRE(info.value().bootMode == BootMode::EDL);
}

SECTION("sahara_reset_success")
{
    MockTransport transport;
    MockLogger logger;
    VirtualSaharaDevice::Behavior beh;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();

    SaharaAdapter adapter(transport, logger);
    REQUIRE_FALSE(adapter.isOpen());

    REQUIRE(adapter.open().isOk());
    REQUIRE(adapter.isOpen());

    auto info = adapter.deviceInfo();
    REQUIRE(info.isOk());
    REQUIRE(info.value().protocolName == "Sahara");
    REQUIRE(info.value().bootMode == BootMode::EDL);

    transport.clearReadQueue();
    transport.resetReadResult();

    device.queueResetResponse();

    auto result = adapter.reset();
    REQUIRE(result.isOk());
    REQUIRE_FALSE(adapter.isOpen());

    auto guardedInfo = adapter.deviceInfo();
    REQUIRE(guardedInfo.isError());
    REQUIRE(guardedInfo.error() == ErrorCode::DeviceDisconnected);
}

}
