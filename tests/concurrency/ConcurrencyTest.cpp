#include <catch2/catch_test_macros.hpp>

#include <mbootcore/runtime/RuntimeObserver.hpp>
#include <mbootcore/session/ISessionObserver.hpp>
#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

using namespace mbootcore;
using namespace mbootcore::runtime;
using namespace mbootcore::session;

// ============================================================
// Minimal flash device for session tests
// ============================================================

class TestFlashDevice : public IFlashDevice {
public:
    std::vector<uint8_t> storage;

    TestFlashDevice() : storage(1024, 0) {}

    Result<void> open() override { m_open = true; return {}; }
    void close() noexcept override { m_open = false; }
    bool isOpen() const noexcept override { return m_open; }
    FlashCapability capabilities() const noexcept override { return FlashCapability::All; }

    Result<GenericDeviceInfo> deviceInfo() override { return GenericDeviceInfo{}; }
    Result<StorageInfo> getStorageInfo() override { return StorageInfo{}; }
    Result<PartitionTable> getPartitions() override { return PartitionTable{}; }

    Result<ByteBuffer> readMemory(uint64_t, size_t) override {
        return ByteBuffer{};
    }
    Result<void> writeMemory(uint64_t, const ByteBuffer&) override { return {}; }
    Result<void> eraseMemory(uint64_t, size_t) override { return {}; }
    Result<void> uploadLoader(const ByteBuffer&) override { return {}; }
    Result<void> reset() override { return {}; }
    Result<void> powerReset() override { return {}; }
    void cancel() noexcept override {}
    void setProgressCallback(ProgressCallback) override {}

    Result<ByteBuffer> readPartition(const std::string&) override { return ByteBuffer{}; }
    Result<void> writePartition(const std::string&, const ByteBuffer&) override { return {}; }
    Result<void> erasePartition(const std::string&) override { return {}; }

private:
    bool m_open{false};
};

// ============================================================
// Test RuntimeObserver
// ============================================================

class TestObserver : public RuntimeObserver {
public:
    std::atomic<int> eventCount{0};
    std::atomic<int> statsCount{0};
    std::atomic<int> healthCount{0};
    ObserverManager* manager{nullptr};
    std::function<void()> onEventAction;
    std::function<void()> onStatsAction;

    void onRuntimeEvent(const RuntimeEvent&) override {
        eventCount++;
        if (onEventAction) onEventAction();
    }
    void onStatisticsUpdated(const RuntimeStatistics&) override {
        statsCount++;
        if (onStatsAction) onStatsAction();
    }
    void onHealthChanged(const RuntimeHealth&) override {
        healthCount++;
    }
};

// ============================================================
// Test ISessionObserver
// ============================================================

class TestSessionObserver : public ISessionObserver {
public:
    std::atomic<int> stateChanges{0};
    std::atomic<int> errors{0};

    void onStateChanged(DeviceSession&, SessionState, SessionState) override { stateChanges++; }
    void onProgress(DeviceSession&, uint64_t, uint64_t) override {}
    void onError(DeviceSession&, ErrorCode, const std::string&) override { errors++; }
    void onCompleted(DeviceSession&, const std::string&) override {}
    void onDisconnected(DeviceSession&) override {}
    void onOperationStarted(DeviceSession&, const std::string&) override {}
    void onOperationFinished(DeviceSession&, const std::string&, bool, std::chrono::milliseconds) override {}
};

// ============================================================
// 1. Observer self-remove during callback
// ============================================================

TEST_CASE("Observer self-remove during callback", "[concurrency][observer]") {
    ObserverManager mgr;
    TestObserver obs;
    obs.manager = &mgr;
    obs.onEventAction = [&]() { mgr.removeObserver(&obs); };

    mgr.addObserver(&obs);

    RuntimeEvent ev;
    ev.type = RuntimeEventType::RuntimeStarted;
    mgr.notifyEvent(ev);

    REQUIRE(obs.eventCount == 1);
}

// ============================================================
// 2. Observer add during callback
// ============================================================

TEST_CASE("Observer add during callback", "[concurrency][observer]") {
    ObserverManager mgr;
    TestObserver obs1;
    TestObserver obs2;
    obs1.onEventAction = [&]() { mgr.addObserver(&obs2); };

    mgr.addObserver(&obs1);

    RuntimeEvent ev;
    ev.type = RuntimeEventType::RuntimeStarted;
    mgr.notifyEvent(ev);

    REQUIRE(obs1.eventCount == 1);
    REQUIRE(obs2.eventCount == 0);

    mgr.notifyEvent(ev);
    REQUIRE(obs2.eventCount == 1);
}

// ============================================================
// 3. Concurrent add/remove observers
// ============================================================

TEST_CASE("Concurrent add/remove observers", "[concurrency][observer][multithread]") {
    ObserverManager mgr;
    std::vector<std::unique_ptr<TestObserver>> observers;
    std::atomic<bool> running{true};

    for (int i = 0; i < 5; ++i) {
        auto obs = std::make_unique<TestObserver>();
        mgr.addObserver(obs.get());
        observers.push_back(std::move(obs));
    }

    auto notifier = std::thread([&]() {
        RuntimeEvent ev;
        ev.type = RuntimeEventType::RuntimeStarted;
        while (running) {
            mgr.notifyEvent(ev);
            std::this_thread::yield();
        }
    });

    auto adder = std::thread([&]() {
        for (int i = 0; i < 10; ++i) {
            auto obs = std::make_unique<TestObserver>();
            mgr.addObserver(obs.get());
            observers.push_back(std::move(obs));
            std::this_thread::yield();
        }
    });

    auto remover = std::thread([&]() {
        for (size_t i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            mgr.removeObserver(observers[i].get());
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;
    notifier.join();
    adder.join();
    remover.join();
}

// ============================================================
// 4. Re-entrant callback
// ============================================================

TEST_CASE("Re-entrant observer callback", "[concurrency][observer][reentrant]") {
    ObserverManager mgr;
    TestObserver obs1;
    TestObserver obs2;

    obs1.onEventAction = [&]() {
        if (obs1.eventCount < 3) {
            RuntimeEvent ev;
            ev.type = RuntimeEventType::RuntimeStarted;
            mgr.notifyEvent(ev);
        }
    };

    mgr.addObserver(&obs1);
    mgr.addObserver(&obs2);

    RuntimeEvent ev;
    ev.type = RuntimeEventType::RuntimeStarted;
    mgr.notifyEvent(ev);

    REQUIRE(obs1.eventCount > 1);
    REQUIRE(obs2.eventCount > 1);
}

// ============================================================
// 5. Repeated notification loops
// ============================================================

TEST_CASE("Repeated notification loops", "[concurrency][observer][loop]") {
    ObserverManager mgr;
    TestObserver obs1;
    TestObserver obs2;

    mgr.addObserver(&obs1);
    mgr.addObserver(&obs2);

    RuntimeEvent ev;
    ev.type = RuntimeEventType::RuntimeStarted;

    for (int i = 0; i < 100; ++i) {
        mgr.notifyEvent(ev);
    }

    REQUIRE(obs1.eventCount == 100);
    REQUIRE(obs2.eventCount == 100);
}

// ============================================================
// 6. Session concurrent observer registration
// ============================================================

TEST_CASE("Session concurrent observer operations", "[concurrency][session][multithread]") {
    discovery::DeviceDescriptor desc;
    desc.vendor = discovery::Vendor::Qualcomm;
    desc.protocolHint = discovery::ProtocolType::Sahara;

    DeviceSession session(desc,
        std::make_unique<TestFlashDevice>(),
        std::make_unique<pipeline::BootPipeline>());

    std::vector<std::unique_ptr<TestSessionObserver>> observers;
    std::atomic<bool> running{true};

    for (int i = 0; i < 5; ++i) {
        observers.push_back(std::make_unique<TestSessionObserver>());
    }

    auto adder = std::thread([&]() {
        while (running) {
            for (auto& obs : observers) {
                session.addObserver(obs.get());
            }
            std::this_thread::yield();
        }
    });

    auto remover = std::thread([&]() {
        while (running) {
            for (auto& obs : observers) {
                session.removeObserver(obs.get());
            }
            std::this_thread::yield();
        }
    });

    auto checker = std::thread([&]() {
        while (running) {
            session.observerCount();
            std::this_thread::yield();
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;
    adder.join();
    remover.join();
    checker.join();
}

// ============================================================
// 7. Observer stress test with many threads
// ============================================================

TEST_CASE("Observer stress test", "[concurrency][stress][multithread]") {
    ObserverManager mgr;
    std::atomic<bool> running{true};

    std::vector<std::unique_ptr<TestObserver>> observers;
    for (int i = 0; i < 10; ++i) {
        observers.push_back(std::make_unique<TestObserver>());
        mgr.addObserver(observers.back().get());
    }

    auto notifier = std::thread([&]() {
        RuntimeEvent ev;
        ev.type = RuntimeEventType::RuntimeStarted;
        while (running) {
            mgr.notifyEvent(ev);
            std::this_thread::yield();
        }
    });

    std::vector<std::thread> adders;
    for (int i = 0; i < 4; ++i) {
        adders.emplace_back([&]() {
            for (int j = 0; j < 5; ++j) {
                auto obs = std::make_unique<TestObserver>();
                mgr.addObserver(obs.get());
                observers.push_back(std::move(obs));
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running = false;
    notifier.join();
    for (auto& t : adders) t.join();
}
