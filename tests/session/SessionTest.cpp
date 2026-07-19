#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/session/ISessionObserver.hpp>
#include <mbootcore/session/SessionLogger.hpp>
#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/session/DeviceSessionFactory.hpp>
#include <mbootcore/session/DeviceManager.hpp>

#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/discovery/VirtualDeviceDetector.hpp>
#include <mbootcore/pipeline/BootPipelineFactory.hpp>

using namespace mbootcore;
using namespace mbootcore::session;
using namespace mbootcore::discovery;

// ============================================================
// Mock Flash Device for testing
// ============================================================

class MockSessionFlashDevice : public IFlashDevice {
public:
    ByteBuffer storage;

    MockSessionFlashDevice() : storage(1024 * 1024, 0) {}

    Result<void> open() override { m_open = true; return {}; }
    void close() noexcept override { m_open = false; }
    bool isOpen() const noexcept override { return m_open; }
    FlashCapability capabilities() const noexcept override { return FlashCapability::All; }

    Result<GenericDeviceInfo> deviceInfo() override {
        return GenericDeviceInfo{};
    }

    Result<StorageInfo> getStorageInfo() override {
        StorageInfo info;
        info.numSectors = 1024;
        info.sectorSize = 1024;
        info.capacity = 1024 * 1024;
        return info;
    }

    Result<PartitionTable> getPartitions() override {
        return PartitionTable{};
    }

    Result<ByteBuffer> readMemory(uint64_t address, size_t size) override {
        if (address + size > storage.size()) {
            return ErrorCode::InvalidArgument;
        }
        ByteBuffer data(storage.begin() + static_cast<std::ptrdiff_t>(address),
                        storage.begin() + static_cast<std::ptrdiff_t>(address + size));
        return data;
    }

    Result<void> writeMemory(uint64_t address, const ByteBuffer& data) override {
        if (address + data.size() > storage.size()) {
            return ErrorCode::InvalidArgument;
        }
        std::copy(data.begin(), data.end(),
                  storage.begin() + static_cast<std::ptrdiff_t>(address));
        m_written += data.size();
        return {};
    }

    Result<void> eraseMemory(uint64_t address, size_t size) override {
        if (address + size > storage.size()) {
            return ErrorCode::InvalidArgument;
        }
        std::fill(storage.begin() + static_cast<std::ptrdiff_t>(address),
                  storage.begin() + static_cast<std::ptrdiff_t>(address + size), 0);
        m_erased += size;
        return {};
    }

    Result<ByteBuffer> readPartition(const std::string&) override {
        return ErrorCode::NotSupported;
    }
    Result<void> writePartition(const std::string&, const ByteBuffer&) override {
        return ErrorCode::NotSupported;
    }
    Result<void> erasePartition(const std::string&) override {
        return ErrorCode::NotSupported;
    }
    Result<void> uploadLoader(const ByteBuffer&) override {
        return {};
    }
    Result<void> reset() override {
        m_resets++;
        return {};
    }
    Result<void> powerReset() override {
        return {};
    }
    void cancel() noexcept override { m_cancelled = true; }
    void setProgressCallback(ProgressCallback) override {}

    uint64_t m_written{0};
    uint64_t m_erased{0};
    int m_resets{0};
    bool m_cancelled{false};

private:
    bool m_open{false};
};

// ============================================================
// Test Observer
// ============================================================

class TestObserver : public ISessionObserver {
public:
    std::vector<std::pair<SessionState, SessionState>> stateChanges;
    std::vector<ErrorCode> errors;
    std::vector<std::string> completedOps;
    std::vector<std::string> startedOps;
    std::vector<std::string> finishedOps;
    int disconnectCount{0};
    int progressCount{0};

    void reset() {
        stateChanges.clear();
        errors.clear();
        completedOps.clear();
        startedOps.clear();
        finishedOps.clear();
        disconnectCount = 0;
        progressCount = 0;
    }

    void onStateChanged(DeviceSession&, SessionState oldState, SessionState newState) override {
        stateChanges.push_back({oldState, newState});
    }

    void onProgress(DeviceSession&, uint64_t, uint64_t) override { ++progressCount; }

    void onError(DeviceSession&, ErrorCode error, const std::string&) override {
        errors.push_back(error);
    }

    void onCompleted(DeviceSession&, const std::string& operation) override {
        completedOps.push_back(operation);
    }

    void onDisconnected(DeviceSession&) override { ++disconnectCount; }

    void onOperationStarted(DeviceSession&, const std::string& operation) override {
        startedOps.push_back(operation);
    }

    void onOperationFinished(DeviceSession&, const std::string& operation, bool, std::chrono::milliseconds) override {
        finishedOps.push_back(operation);
    }
};

// ============================================================
// Test Factory
// ============================================================

class SessionTestFactory : public IProtocolFactory {
public:
    ProtocolType protocolType() const override { return ProtocolType::Sahara; }

    std::unique_ptr<IFlashDevice> createFlashDevice(const DeviceDescriptor&) override {
        return std::make_unique<MockSessionFlashDevice>();
    }

    std::unique_ptr<pipeline::BootPipeline> createPipeline(const DeviceDescriptor&) override {
        return std::make_unique<pipeline::BootPipeline>();
    }
};

// ============================================================
// Test Class
// ============================================================

void testSessionStateEnumValues();
void testSessionStateTransitions();
void testSessionConfigDefaults();
void testLoggerDefaultConstruction();
void testLoggerInfoLogging();
void testLoggerLevels();
void testLoggerClear();
void testLoggerEntryCount();
void testLoggerLastEntries();
void testLoggerEntriesByCategory();
void testLoggerEntriesByLevel();
void testLoggerExportText();
void testLoggerExportJson();
void testLoggerMaxEntries();
void testLoggerCritical();
void testSessionCreate();
void testSessionConnect();
void testSessionDisconnect();
void testSessionReconnect();
void testSessionStateAfterConnect();
void testSessionIsConnected();
void testSessionIsBusy();
void testSessionIsTerminal();
void testSessionStateString();
void testSessionMultipleConnect();
void testSessionConnectAfterTerminal();
void testSessionDescriptorAccessor();
void testSessionDeviceDescriptor();
void testSessionFlashDeviceAccess();
void testSessionSessionId();
void testSessionReadMemory();
void testSessionWriteMemory();
void testSessionEraseMemory();
void testSessionResetDevice();
void testSessionReadWriteConsistency();
void testSessionReadOutOfRange();
void testSessionWriteOutOfRange();
void testSessionOperationWhileBusy();
void testObserverAddRemove();
void testObserverStateChanged();
void testObserverError();
void testObserverCompleted();
void testObserverDisconnect();
void testObserverOperationStarted();
void testObserverOperationFinished();
void testObserverMultipleObservers();
void testObserverHasObserver();
void testObserverRemoveDuringSession();
void testSessionCancel();
void testSessionCancelState();
void testSessionCancelBeforeConnect();
void testStatisticsAfterRead();
void testStatisticsAfterWrite();
void testStatisticsAfterMultipleOps();
void testStatisticsHasStartTime();
void testStatisticsRetries();
void testFactoryCreateSession();
void testFactoryCreateSessionNoMatchingProtocol();
void testFactoryRegistryIntegration();
void testDeviceManagerEmpty();
void testDeviceManagerCreateSession();
void testDeviceManagerCreateMultiple();
void testDeviceManagerRemoveSession();
void testDeviceManagerFindById();
void testDeviceManagerFindByVendor();
void testDeviceManagerFindByProtocol();
void testDeviceManagerFindByDescriptor();
void testDeviceManagerFindByState();
void testDeviceManagerActiveSessions();
void testDeviceManagerRemoveAll();
void testDeviceManagerSessionCount();
void testDeviceManagerRemoveInvalid();
void testHistoryAfterOperations();
void testHistoryDisable();
void testSessionFromDeviceDescriptor();
void testStressManySessions();
void testStressObserverNotifications();
void testStressRapidConnectDisconnect();
void testSessionRetry();
void testSessionRetryNotInErrorState();
TEST_CASE("SessionTest", "[session]") {

    // --- SessionState enum tests ---
    SECTION("testSessionStateEnumValues") {
        testSessionStateEnumValues();
    }
    SECTION("testSessionStateTransitions") {
        testSessionStateTransitions();
    }

    // --- SessionConfig tests ---
    SECTION("testSessionConfigDefaults") {
        testSessionConfigDefaults();
    }

    // --- SessionLogger tests ---
    SECTION("testLoggerDefaultConstruction") {
        testLoggerDefaultConstruction();
    }
    SECTION("testLoggerInfoLogging") {
        testLoggerInfoLogging();
    }
    SECTION("testLoggerLevels") {
        testLoggerLevels();
    }
    SECTION("testLoggerClear") {
        testLoggerClear();
    }
    SECTION("testLoggerEntryCount") {
        testLoggerEntryCount();
    }
    SECTION("testLoggerLastEntries") {
        testLoggerLastEntries();
    }
    SECTION("testLoggerEntriesByCategory") {
        testLoggerEntriesByCategory();
    }
    SECTION("testLoggerEntriesByLevel") {
        testLoggerEntriesByLevel();
    }
    SECTION("testLoggerExportText") {
        testLoggerExportText();
    }
    SECTION("testLoggerExportJson") {
        testLoggerExportJson();
    }
    SECTION("testLoggerMaxEntries") {
        testLoggerMaxEntries();
    }
    SECTION("testLoggerCritical") {
        testLoggerCritical();
    }

    // --- DeviceSession tests ---
    SECTION("testSessionCreate") {
        testSessionCreate();
    }
    SECTION("testSessionConnect") {
        testSessionConnect();
    }
    SECTION("testSessionDisconnect") {
        testSessionDisconnect();
    }
    SECTION("testSessionReconnect") {
        testSessionReconnect();
    }
    SECTION("testSessionStateAfterConnect") {
        testSessionStateAfterConnect();
    }
    SECTION("testSessionIsConnected") {
        testSessionIsConnected();
    }
    SECTION("testSessionIsBusy") {
        testSessionIsBusy();
    }
    SECTION("testSessionIsTerminal") {
        testSessionIsTerminal();
    }
    SECTION("testSessionStateString") {
        testSessionStateString();
    }
    SECTION("testSessionMultipleConnect") {
        testSessionMultipleConnect();
    }
    SECTION("testSessionConnectAfterTerminal") {
        testSessionConnectAfterTerminal();
    }
    SECTION("testSessionDescriptorAccessor") {
        testSessionDescriptorAccessor();
    }
    SECTION("testSessionDeviceDescriptor") {
        testSessionDeviceDescriptor();
    }
    SECTION("testSessionFlashDeviceAccess") {
        testSessionFlashDeviceAccess();
    }
    SECTION("testSessionSessionId") {
        testSessionSessionId();
    }

    // --- DeviceSession operations ---
    SECTION("testSessionReadMemory") {
        testSessionReadMemory();
    }
    SECTION("testSessionWriteMemory") {
        testSessionWriteMemory();
    }
    SECTION("testSessionEraseMemory") {
        testSessionEraseMemory();
    }
    SECTION("testSessionResetDevice") {
        testSessionResetDevice();
    }
    SECTION("testSessionReadWriteConsistency") {
        testSessionReadWriteConsistency();
    }
    SECTION("testSessionReadOutOfRange") {
        testSessionReadOutOfRange();
    }
    SECTION("testSessionWriteOutOfRange") {
        testSessionWriteOutOfRange();
    }
    SECTION("testSessionOperationWhileBusy") {
        testSessionOperationWhileBusy();
    }

    // --- Observer tests ---
    SECTION("testObserverAddRemove") {
        testObserverAddRemove();
    }
    SECTION("testObserverStateChanged") {
        testObserverStateChanged();
    }
    SECTION("testObserverError") {
        testObserverError();
    }
    SECTION("testObserverCompleted") {
        testObserverCompleted();
    }
    SECTION("testObserverDisconnect") {
        testObserverDisconnect();
    }
    SECTION("testObserverOperationStarted") {
        testObserverOperationStarted();
    }
    SECTION("testObserverOperationFinished") {
        testObserverOperationFinished();
    }
    SECTION("testObserverMultipleObservers") {
        testObserverMultipleObservers();
    }
    SECTION("testObserverHasObserver") {
        testObserverHasObserver();
    }
    SECTION("testObserverRemoveDuringSession") {
        testObserverRemoveDuringSession();
    }

    // --- Cancellation tests ---
    SECTION("testSessionCancel") {
        testSessionCancel();
    }
    SECTION("testSessionCancelState") {
        testSessionCancelState();
    }
    SECTION("testSessionCancelBeforeConnect") {
        testSessionCancelBeforeConnect();
    }

    // --- Statistics tests ---
    SECTION("testStatisticsAfterRead") {
        testStatisticsAfterRead();
    }
    SECTION("testStatisticsAfterWrite") {
        testStatisticsAfterWrite();
    }
    SECTION("testStatisticsAfterMultipleOps") {
        testStatisticsAfterMultipleOps();
    }
    SECTION("testStatisticsHasStartTime") {
        testStatisticsHasStartTime();
    }
    SECTION("testStatisticsRetries") {
        testStatisticsRetries();
    }

    // --- DeviceSessionFactory tests ---
    SECTION("testFactoryCreateSession") {
        testFactoryCreateSession();
    }
    SECTION("testFactoryCreateSessionNoMatchingProtocol") {
        testFactoryCreateSessionNoMatchingProtocol();
    }
    SECTION("testFactoryRegistryIntegration") {
        testFactoryRegistryIntegration();
    }

    // --- DeviceManager tests ---
    SECTION("testDeviceManagerEmpty") {
        testDeviceManagerEmpty();
    }
    SECTION("testDeviceManagerCreateSession") {
        testDeviceManagerCreateSession();
    }
    SECTION("testDeviceManagerCreateMultiple") {
        testDeviceManagerCreateMultiple();
    }
    SECTION("testDeviceManagerRemoveSession") {
        testDeviceManagerRemoveSession();
    }
    SECTION("testDeviceManagerFindById") {
        testDeviceManagerFindById();
    }
    SECTION("testDeviceManagerFindByVendor") {
        testDeviceManagerFindByVendor();
    }
    SECTION("testDeviceManagerFindByProtocol") {
        testDeviceManagerFindByProtocol();
    }
    SECTION("testDeviceManagerFindByDescriptor") {
        testDeviceManagerFindByDescriptor();
    }
    SECTION("testDeviceManagerFindByState") {
        testDeviceManagerFindByState();
    }
    SECTION("testDeviceManagerActiveSessions") {
        testDeviceManagerActiveSessions();
    }
    SECTION("testDeviceManagerRemoveAll") {
        testDeviceManagerRemoveAll();
    }
    SECTION("testDeviceManagerSessionCount") {
        testDeviceManagerSessionCount();
    }
    SECTION("testDeviceManagerRemoveInvalid") {
        testDeviceManagerRemoveInvalid();
    }

    // --- History tests ---
    SECTION("testHistoryAfterOperations") {
        testHistoryAfterOperations();
    }
    SECTION("testHistoryDisable") {
        testHistoryDisable();
    }

    // --- MockDeviceSession tests ---
    SECTION("testSessionFromDeviceDescriptor") {
        testSessionFromDeviceDescriptor();
    }

    // --- Stress tests ---
    SECTION("testStressManySessions") {
        testStressManySessions();
    }
    SECTION("testStressObserverNotifications") {
        testStressObserverNotifications();
    }
    SECTION("testStressRapidConnectDisconnect") {
        testStressRapidConnectDisconnect();
    }

    // --- Recovery tests ---
    SECTION("testSessionRetry") {
        testSessionRetry();
    }
    SECTION("testSessionRetryNotInErrorState") {
        testSessionRetryNotInErrorState();
    }
}

// ============================================================
// SessionState Tests
// ============================================================

void testSessionStateEnumValues() {
    REQUIRE(static_cast<int>(SessionState::Disconnected) == 0);
    REQUIRE(static_cast<int>(SessionState::Ready) == 5);
    REQUIRE(static_cast<int>(SessionState::Finished) == 11);
    REQUIRE(static_cast<int>(SessionState::Error) == 12);
    REQUIRE(static_cast<int>(SessionState::Cancelled) == 13);
}

void testSessionStateTransitions() {
    // Verify state ordering for terminal detection
    REQUIRE(static_cast<int>(SessionState::Finished) > static_cast<int>(SessionState::Ready));
    REQUIRE(static_cast<int>(SessionState::Error) > static_cast<int>(SessionState::Ready));
    REQUIRE(static_cast<int>(SessionState::Cancelled) > static_cast<int>(SessionState::Ready));
}

// ============================================================
// SessionConfig Tests
// ============================================================

void testSessionConfigDefaults() {
    SessionConfig cfg;
    REQUIRE(cfg.connectTimeout.count() == 5000);
    REQUIRE(cfg.operationTimeout.count() == 30000);
    REQUIRE(cfg.maxRetries == 3);
    REQUIRE(cfg.maxRecoveryAttempts == 2);
    REQUIRE(cfg.enableLogging);
    REQUIRE(cfg.enableHistory);
    REQUIRE(cfg.enableStatistics);
    REQUIRE(cfg.enableAutoRecovery);
    REQUIRE(!cfg.cancelOnError);
}

// ============================================================
// SessionLogger Tests
// ============================================================

void testLoggerDefaultConstruction() {
    SessionLogger logger;
    REQUIRE(logger.sessionName() == "Default");
    REQUIRE(logger.entryCount() == std::size_t(0));
    REQUIRE(logger.maxEntries() == std::size_t(10000));
}

void testLoggerInfoLogging() {
    SessionLogger logger("test");
    logger.info("TestCat", "Test message");
    REQUIRE(logger.entryCount() == std::size_t(1));
}

void testLoggerLevels() {
    SessionLogger logger("test");
    logger.debug("cat", "debug msg");
    logger.info("cat", "info msg");
    logger.warning("cat", "warn msg");
    logger.error("cat", "err msg");
    logger.critical("cat", "crit msg");
    REQUIRE(logger.entryCount() == std::size_t(5));

    auto errors = logger.entriesByLevel(session::LogLevel::Error);
    REQUIRE(errors.size() == std::size_t(1));
    REQUIRE(errors[0].message == "err msg");
}

void testLoggerClear() {
    SessionLogger logger("test");
    logger.info("cat", "msg1");
    logger.info("cat", "msg2");
    REQUIRE(logger.entryCount() == std::size_t(2));
    logger.clear();
    REQUIRE(logger.entryCount() == std::size_t(0));
}

void testLoggerEntryCount() {
    SessionLogger logger("test");
    REQUIRE(logger.entryCount() == std::size_t(0));
    logger.info("a", "1");
    REQUIRE(logger.entryCount() == std::size_t(1));
    logger.info("a", "2");
    REQUIRE(logger.entryCount() == std::size_t(2));
}

void testLoggerLastEntries() {
    SessionLogger logger("test");
    logger.info("a", "first");
    logger.info("a", "second");
    logger.info("a", "third");

    auto last2 = logger.lastEntries(2);
    REQUIRE(last2.size() == std::size_t(2));
    REQUIRE(last2[0].message == "second");
    REQUIRE(last2[1].message == "third");
}

void testLoggerEntriesByCategory() {
    SessionLogger logger("test");
    logger.info("USB", "usb msg");
    logger.info("Session", "session msg");
    logger.info("USB", "another usb");

    auto usbEntries = logger.entriesByCategory("USB");
    REQUIRE(usbEntries.size() == std::size_t(2));
}

void testLoggerEntriesByLevel() {
    SessionLogger logger("test");
    logger.info("a", "info");
    logger.warning("a", "warn");
    logger.info("a", "info2");

    auto infoEntries = logger.entriesByLevel(session::LogLevel::Info);
    REQUIRE(infoEntries.size() == std::size_t(2));
}

void testLoggerExportText() {
    SessionLogger logger("test");
    logger.info("T", "hello");
    auto text = logger.exportText();
    REQUIRE(!text.empty());
    REQUIRE(text.find("INFO") != std::string::npos);
    REQUIRE(text.find("hello") != std::string::npos);
}

void testLoggerExportJson() {
    SessionLogger logger("test");
    logger.info("T", "hello");
    auto json = logger.exportJson();
    REQUIRE(!json.empty());
    REQUIRE(json.find("\"message\"") != std::string::npos);
    REQUIRE(json.find("hello") != std::string::npos);
    REQUIRE(json.find("\"entries\"") != std::string::npos);
}

void testLoggerMaxEntries() {
    SessionLogger logger("test");
    logger.setMaxEntries(3);
    logger.info("a", "1");
    logger.info("a", "2");
    logger.info("a", "3");
    logger.info("a", "4");
    REQUIRE(logger.entryCount() == std::size_t(3));
}

void testLoggerCritical() {
    SessionLogger logger("test");
    logger.critical("CRIT", "critical error");
    REQUIRE(logger.entryCount() == std::size_t(1));
    auto crits = logger.entriesByLevel(session::LogLevel::Fatal);
    REQUIRE(crits.size() == std::size_t(1));
}

// ============================================================
// DeviceSession Tests
// ============================================================

DeviceDescriptor makeTestDescriptor() {
    DeviceDescriptor desc;
    desc.vendor = Vendor::Qualcomm;
    desc.bootMode = discovery::BootMode::EDL;
    desc.protocolHint = ProtocolType::Sahara;
    desc.usbVid = 0x05C6;
    desc.usbPid = 0x9008;
    desc.friendlyName = "TestDevice";
    desc.connectionPath = "usb:05C6:9008";
    return desc;
}

void testSessionCreate() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    auto* rawFlash = flash.get();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(session.state() == SessionState::Disconnected);
    REQUIRE(!session.isConnected());
    REQUIRE(!session.isBusy());
    REQUIRE(!session.isTerminal());
    REQUIRE(session.flashDevice() == rawFlash);
}

void testSessionConnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    auto result = session.connect();
    REQUIRE(result.isOk());
    REQUIRE(session.isConnected());
}

void testSessionDisconnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();
    REQUIRE(session.isConnected());
    session.disconnect();
    REQUIRE(session.state() == SessionState::Disconnected);
    REQUIRE(!session.isConnected());
}

void testSessionReconnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();
    REQUIRE(session.isConnected());
    auto result = session.reconnect();
    REQUIRE(result.isOk());
    REQUIRE(session.isConnected());
}

void testSessionStateAfterConnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(session.state() == SessionState::Disconnected);
    session.connect();
    REQUIRE(session.state() == SessionState::Ready);
}

void testSessionIsConnected() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(!session.isConnected());
    session.connect();
    REQUIRE(session.isConnected());
    session.disconnect();
    REQUIRE(!session.isConnected());
}

void testSessionIsBusy() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(!session.isBusy());
    session.connect();
    REQUIRE(!session.isBusy());
}

void testSessionIsTerminal() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(!session.isTerminal());
}

void testSessionStateString() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(session.stateString() == "Disconnected");
    session.connect();
    REQUIRE(session.stateString() == "Ready");
}

void testSessionMultipleConnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    auto r1 = session.connect();
    REQUIRE(r1.isOk());
    auto r2 = session.connect();
    REQUIRE(r2.isOk());
    REQUIRE(session.isConnected());
}

void testSessionConnectAfterTerminal() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.disconnect();
    // After disconnect, should be able to reconnect
    auto result = session.connect();
    REQUIRE(result.isOk());
}

void testSessionDescriptorAccessor() {
    auto desc = makeTestDescriptor();
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(desc, std::move(flash));
    const auto& d = session.descriptor();
    REQUIRE(d.vendor == Vendor::Qualcomm);
    REQUIRE(d.usbVid == 0x05C6);
}

void testSessionDeviceDescriptor() {
    auto desc = makeTestDescriptor();
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(desc, std::move(flash));
    auto pipe = session.pipeline();
    REQUIRE(pipe == nullptr);
}

void testSessionFlashDeviceAccess() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    auto* raw = flash.get();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(session.flashDevice() == raw);
}

void testSessionSessionId() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    REQUIRE(!session.sessionId().empty());
    REQUIRE(session.sessionId().find("usb:") != std::string::npos);
}

// ============================================================
// DeviceSession Operations
// ============================================================

void testSessionReadMemory() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    auto result = session.readMemory(0, 100);
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == std::size_t(100));
}

void testSessionWriteMemory() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    ByteBuffer data(100, 0xAB);
    auto result = session.writeMemory(0, data);
    REQUIRE(result.isOk());
}

void testSessionEraseMemory() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    auto result = session.eraseMemory(0, 100);
    REQUIRE(result.isOk());
}

void testSessionResetDevice() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    auto* rawFlash = static_cast<MockSessionFlashDevice*>(flash.get());
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    auto result = session.resetDevice();
    REQUIRE(result.isOk());
    REQUIRE(rawFlash->m_resets == 1);
}

void testSessionReadWriteConsistency() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    ByteBuffer writeData({1, 2, 3, 4, 5});
    auto writeResult = session.writeMemory(0, writeData);
    REQUIRE(writeResult.isOk());

    auto readResult = session.readMemory(0, 5);
    REQUIRE(readResult.isOk());
    REQUIRE(readResult.value() == writeData);
}

void testSessionReadOutOfRange() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    auto result = session.readMemory(1048576, 100);
    REQUIRE(result.isError());
}

void testSessionWriteOutOfRange() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    ByteBuffer data(100, 0);
    auto result = session.writeMemory(1048576, data);
    REQUIRE(result.isError());
}

void testSessionOperationWhileBusy() {
    // After a read, session is back to Ready, so this tests rapid sequential ops
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    auto r1 = session.readMemory(0, 10);
    REQUIRE(r1.isOk());
    auto r2 = session.readMemory(10, 10);
    REQUIRE(r2.isOk());
}

// ============================================================
// Observer Tests
// ============================================================

void testObserverAddRemove() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;

    REQUIRE(session.observerCount() == std::size_t(0));
    session.addObserver(&obs);
    REQUIRE(session.observerCount() == std::size_t(1));
    session.removeObserver(&obs);
    REQUIRE(session.observerCount() == std::size_t(0));
}

void testObserverStateChanged() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();
    REQUIRE(obs.stateChanges.size() >= 2);
    session.removeObserver(&obs);
}

void testObserverError() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();

    // Test that readMemory can fail and observer gets notified
    // Use an address that's definitely out of range
    auto result = session.readMemory(1048576, 100);
    REQUIRE(result.isError() == true);
    REQUIRE(obs.errors.size() == std::size_t(1));
    session.removeObserver(&obs);
}

void testObserverCompleted() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();
    REQUIRE(!obs.completedOps.empty());
    session.removeObserver(&obs);
}

void testObserverDisconnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();
    REQUIRE(obs.disconnectCount == 0);
    session.disconnect();
    REQUIRE(obs.disconnectCount == 1);
    session.removeObserver(&obs);
}

void testObserverOperationStarted() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();
    REQUIRE(!obs.startedOps.empty());
    session.removeObserver(&obs);
}

void testObserverOperationFinished() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();
    REQUIRE(!obs.finishedOps.empty());
    session.removeObserver(&obs);
}

void testObserverMultipleObservers() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs1, obs2;

    session.addObserver(&obs1);
    session.addObserver(&obs2);
    REQUIRE(session.observerCount() == std::size_t(2));

    session.connect();
    REQUIRE(!obs1.startedOps.empty());
    REQUIRE(!obs2.startedOps.empty());
    session.removeObserver(&obs1);
    session.removeObserver(&obs2);
}

void testObserverHasObserver() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;

    REQUIRE(!session.hasObserver(&obs));
    session.addObserver(&obs);
    REQUIRE(session.hasObserver(&obs));
    session.removeObserver(&obs);
    REQUIRE(!session.hasObserver(&obs));
}

void testObserverRemoveDuringSession() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);
    session.removeObserver(&obs);
    REQUIRE(session.observerCount() == std::size_t(0));
}

// ============================================================
// Cancellation Tests
// ============================================================

void testSessionCancel() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    session.cancel();
    REQUIRE(session.isCancelled());
}

void testSessionCancelState() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.cancel();

    // Cancel before connect should cause operations to fail
    auto result = session.readMemory(0, 100);
    REQUIRE(result.isError());
}

void testSessionCancelBeforeConnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.cancel();

    // Cancelled is a terminal state — connect() should reject
    auto result = session.connect();
    REQUIRE(result.isError());
    REQUIRE(session.state() == SessionState::Cancelled);
    REQUIRE(session.isTerminal());
}

// ============================================================
// Statistics Tests
// ============================================================

void testStatisticsAfterRead() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    session.readMemory(0, 100);
    const auto& stats = session.statistics();
    REQUIRE(stats.bytesRead == uint64_t(100));
    REQUIRE(stats.readOps == uint32_t(1));
}

void testStatisticsAfterWrite() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    ByteBuffer data(200, 0xFF);
    session.writeMemory(0, data);
    const auto& stats = session.statistics();
    REQUIRE(stats.bytesWritten == uint64_t(200));
    REQUIRE(stats.writeOps == uint32_t(1));
}

void testStatisticsAfterMultipleOps() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    session.readMemory(0, 100);
    session.readMemory(100, 200);
    ByteBuffer data(50, 0);
    session.writeMemory(0, data);
    session.eraseMemory(0, 64);

    const auto& stats = session.statistics();
    REQUIRE(stats.bytesRead == uint64_t(300));
    REQUIRE(stats.bytesWritten == uint64_t(50));
    REQUIRE(stats.bytesErased == uint64_t(64));
    REQUIRE(stats.readOps == uint32_t(2));
    REQUIRE(stats.writeOps == uint32_t(1));
    REQUIRE(stats.eraseOps == uint32_t(1));
}

void testStatisticsHasStartTime() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    const auto& stats = session.statistics();
    REQUIRE(stats.elapsedTime.count() >= 0);
}

void testStatisticsRetries() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    const auto& stats = session.statistics();
    REQUIRE(stats.retries == uint32_t(0));
}

// ============================================================
// DeviceSessionFactory Tests
// ============================================================

void testFactoryCreateSession() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    reg.registerNegotiator(std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara));

    DeviceSessionFactory factory(reg);
    auto desc = makeTestDescriptor();
    auto result = factory.createSession(desc);
    REQUIRE(result.isOk());
    REQUIRE(result.value() != nullptr);
    REQUIRE(result.value()->descriptor().vendor == Vendor::Qualcomm);
}

void testFactoryCreateSessionNoMatchingProtocol() {
    ProtocolRegistry reg;
    // No factories registered
    DeviceSessionFactory factory(reg);

    auto desc = makeTestDescriptor();
    auto result = factory.createSession(desc);
    REQUIRE(result.isError());
}

void testFactoryRegistryIntegration() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());

    DeviceSessionFactory factory(reg);
    REQUIRE(&factory.registry() == &reg);
}

// ============================================================
// DeviceManager Tests
// ============================================================

void testDeviceManagerEmpty() {
    ProtocolRegistry reg;
    DeviceManager mgr(reg);
    REQUIRE(mgr.sessionCount() == std::size_t(0));
    REQUIRE(mgr.activeCount() == std::size_t(0));
    REQUIRE(mgr.activeSessions().empty());
    REQUIRE(mgr.allSessions().empty());
}

void testDeviceManagerCreateSession() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    auto result = mgr.createSession(makeTestDescriptor());
    REQUIRE(result.isOk());
    REQUIRE(result.value() != nullptr);
    REQUIRE(mgr.sessionCount() == std::size_t(1));
}

void testDeviceManagerCreateMultiple() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    auto d1 = mgr.createSession(makeTestDescriptor());
    REQUIRE(d1.isOk());

    auto d2desc = makeTestDescriptor();
    d2desc.usbPid = 0x900E;
    d2desc.connectionPath = "usb:05C6:900E";
    auto d2 = mgr.createSession(d2desc);
    REQUIRE(d2.isOk());

    REQUIRE(mgr.sessionCount() == std::size_t(2));
}

void testDeviceManagerRemoveSession() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    auto* session = mgr.createSession(makeTestDescriptor()).value();
    REQUIRE(mgr.sessionCount() == std::size_t(1));

    auto result = mgr.removeSession(session->sessionId());
    REQUIRE(result.isOk());
    REQUIRE(mgr.sessionCount() == std::size_t(0));
}

void testDeviceManagerFindById() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    auto* session = mgr.createSession(makeTestDescriptor()).value();
    auto* found = mgr.findById(session->sessionId());
    REQUIRE(found != nullptr);
    REQUIRE(found == session);

    auto* notFound = mgr.findById("nonexistent");
    REQUIRE(notFound == nullptr);
}

void testDeviceManagerFindByVendor() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    mgr.createSession(makeTestDescriptor());
    auto samsungDesc = makeTestDescriptor();
    samsungDesc.vendor = Vendor::Samsung;
    samsungDesc.usbVid = 0x04E8;
    samsungDesc.connectionPath = "usb:04E8:685D";
    mgr.createSession(samsungDesc);

    auto qualcomm = mgr.findByVendor(Vendor::Qualcomm);
    REQUIRE(qualcomm.size() == std::size_t(1));

    auto samsung = mgr.findByVendor(Vendor::Samsung);
    REQUIRE(samsung.size() == std::size_t(1));

    auto unknown = mgr.findByVendor(Vendor::MediaTek);
    REQUIRE(unknown.empty());
}

void testDeviceManagerFindByProtocol() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    mgr.createSession(makeTestDescriptor());

    auto sahara = mgr.findByProtocol(ProtocolType::Sahara);
    REQUIRE(sahara.size() == std::size_t(1));

    auto firehose = mgr.findByProtocol(ProtocolType::Firehose);
    REQUIRE(firehose.empty());
}

void testDeviceManagerFindByDescriptor() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    auto desc = makeTestDescriptor();
    mgr.createSession(desc);

    auto* found = mgr.findByDescriptor(desc);
    REQUIRE(found != nullptr);

    DeviceDescriptor other;
    other.connectionPath = "nonexistent";
    auto* notFound = mgr.findByDescriptor(other);
    REQUIRE(notFound == nullptr);
}

void testDeviceManagerFindByState() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    mgr.createSession(makeTestDescriptor());
    mgr.createSession(makeTestDescriptor());

    auto disconnected = mgr.findByState(SessionState::Disconnected);
    REQUIRE(disconnected.size() == std::size_t(2));
}

void testDeviceManagerActiveSessions() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    mgr.createSession(makeTestDescriptor());
    REQUIRE(mgr.activeCount() == std::size_t(0));  // not connected yet

    // Connect all
    mgr.connectAll();
    REQUIRE(mgr.activeCount() == std::size_t(1));
}

void testDeviceManagerRemoveAll() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    mgr.createSession(makeTestDescriptor());
    mgr.createSession(makeTestDescriptor());
    REQUIRE(mgr.sessionCount() == std::size_t(2));

    mgr.removeAll();
    REQUIRE(mgr.sessionCount() == std::size_t(0));
}

void testDeviceManagerSessionCount() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    REQUIRE(mgr.sessionCount() == std::size_t(0));
    mgr.createSession(makeTestDescriptor());
    REQUIRE(mgr.sessionCount() == std::size_t(1));
    mgr.createSession(makeTestDescriptor());
    REQUIRE(mgr.sessionCount() == std::size_t(2));
}

void testDeviceManagerRemoveInvalid() {
    ProtocolRegistry reg;
    DeviceManager mgr(reg);

    auto result = mgr.removeSession("nonexistent");
    REQUIRE(result.isError());
}

// ============================================================
// History Tests
// ============================================================

void testHistoryAfterOperations() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();
    session.readMemory(0, 100);
    ByteBuffer data(50, 0);
    session.writeMemory(0, data);

    const auto& history = session.history();
    REQUIRE(history.size() >= 2);
}

void testHistoryDisable() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.config().enableHistory = false;
    session.connect();
    session.readMemory(0, 100);

    const auto& history = session.history();
    REQUIRE(history.empty());
}

// ============================================================
// Descriptor to Session tests
// ============================================================

void testSessionFromDeviceDescriptor() {
    auto desc = makeTestDescriptor();
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(desc, std::move(flash));

    REQUIRE(session.descriptor().vendor == desc.vendor);
    REQUIRE(session.descriptor().protocolHint == desc.protocolHint);
    REQUIRE(session.descriptor().usbVid == desc.usbVid);
}

// ============================================================
// Stress Tests
// ============================================================

void testStressManySessions() {
    ProtocolRegistry reg;
    reg.registerFactory(std::make_unique<SessionTestFactory>());
    DeviceManager mgr(reg);

    for (int i = 0; i < 50; ++i) {
        auto desc = makeTestDescriptor();
        desc.connectionPath = "usb:05C6:" + std::to_string(0x9000 + i);
        auto result = mgr.createSession(desc);
        REQUIRE(result.isOk());
    }

    REQUIRE(mgr.sessionCount() == std::size_t(50));
    mgr.removeAll();
    REQUIRE(mgr.sessionCount() == std::size_t(0));
}

void testStressObserverNotifications() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    TestObserver obs;
    session.addObserver(&obs);

    session.connect();
    // Do many operations
    for (int i = 0; i < 20; ++i) {
        session.readMemory(static_cast<uint64_t>(i) * 100, 50);
    }

    REQUIRE(obs.startedOps.size() >= 20);
    REQUIRE(obs.finishedOps.size() >= 20);
    session.removeObserver(&obs);
}

void testStressRapidConnectDisconnect() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));

    for (int i = 0; i < 20; ++i) {
        auto r = session.connect();
        REQUIRE(r.isOk());
        session.disconnect();
    }

    REQUIRE(session.state() == SessionState::Disconnected);
}

// ============================================================
// Recovery Tests
// ============================================================

void testSessionRetry() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));

    // Session not in error state, can't retry
    auto result = session.retry();
    REQUIRE(result.isError());
}

void testSessionRetryNotInErrorState() {
    auto flash = std::make_unique<MockSessionFlashDevice>();
    DeviceSession session(makeTestDescriptor(), std::move(flash));
    session.connect();

    auto result = session.retry();
    REQUIRE(result.isError());
}

