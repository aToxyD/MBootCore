#include <mbootcore/stress/VirtualStressEnvironment.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <algorithm>
#include <unordered_map>

namespace mbootcore { namespace stress {

namespace {

struct VirtualDevice {
    VirtualDeviceConfig config;
    bool connected{false};
    uint32_t operationsCompleted{0};
};

uint32_t randomUint32(uint32_t min, uint32_t max) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(rng);
}

bool probabilityCheck(uint32_t probability) {
    if (probability == 0) return false;
    if (probability >= 100) return true;
    return randomUint32(0, 99) < probability;
}

}

struct VirtualStressEnvironment::Impl {
    StressTestConfig config;
    std::vector<VirtualDevice> devices;
    std::atomic<StressTestState> currentState{StressTestState::Idle};
    mutable std::mutex mtx;
    std::atomic<uint32_t> totalOps{0};
    std::atomic<uint32_t> successOps{0};
    std::atomic<uint32_t> failedOps{0};
    std::atomic<uint32_t> timeoutOps{0};
    std::atomic<uint32_t> recoveredOps{0};
    std::atomic<uint64_t> durationMs{0};
    std::atomic<FailureMode> currentFailure{FailureMode::None};
    std::vector<std::string> errorList;
    std::vector<std::thread> workers;

    void simulateOperation(const VirtualDevice& dev, uint32_t) {
        if (probabilityCheck(dev.config.disconnectProbability)) {
            failedOps.fetch_add(1);
            return;
        }
        if (currentFailure.load() == FailureMode::Timeout &&
            probabilityCheck(config.randomFailureRate > 0 ? config.randomFailureRate : 10)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.timeoutMs + 100));
            timeoutOps.fetch_add(1);
            return;
        }
        if (currentFailure.load() == FailureMode::TransportCorruption &&
            probabilityCheck(config.randomFailureRate > 0 ? config.randomFailureRate : 10)) {
            failedOps.fetch_add(1);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        successOps.fetch_add(1);
    }

    void runOperationsOnDevice(VirtualDevice& dev) {
        for (uint32_t i = 0; i < config.operationsPerDevice; ++i) {
            if (currentState.load() != StressTestState::Running) return;
            totalOps.fetch_add(1);
            simulateOperation(dev, i);
        }
        dev.operationsCompleted = config.operationsPerDevice;
    }
};

VirtualStressEnvironment::VirtualStressEnvironment()
    : m_impl(std::make_unique<Impl>()) {}

VirtualStressEnvironment::~VirtualStressEnvironment() = default;
VirtualStressEnvironment::VirtualStressEnvironment(VirtualStressEnvironment&&) noexcept = default;
VirtualStressEnvironment& VirtualStressEnvironment::operator=(VirtualStressEnvironment&&) noexcept = default;

Result<void> VirtualStressEnvironment::initialize(const StressTestConfig& config) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->config = config;
    m_impl->currentState.store(StressTestState::Idle);
    m_impl->devices.clear();
    m_impl->totalOps.store(0);
    m_impl->successOps.store(0);
    m_impl->failedOps.store(0);
    m_impl->timeoutOps.store(0);
    m_impl->recoveredOps.store(0);
    m_impl->durationMs.store(0);
    m_impl->errorList.clear();
    return {};
}

Result<void> VirtualStressEnvironment::createDevices(uint32_t count) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->devices.clear();
    m_impl->devices.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        VirtualDeviceConfig cfg;
        cfg.deviceId = "VirtualDevice_" + std::to_string(i);
        cfg.type = (i % 2 == 0) ? VirtualDeviceType::Sahara : VirtualDeviceType::Firehose;
        m_impl->devices.push_back({cfg, false, 0});
    }
    return {};
}

Result<void> VirtualStressEnvironment::addDevice(const VirtualDeviceConfig& config) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->devices.push_back({config, false, 0});
    return {};
}

Result<void> VirtualStressEnvironment::runStressTest() {
    if (m_impl->currentState.load() != StressTestState::Idle &&
        m_impl->currentState.load() != StressTestState::Completed) {
        return ErrorCode::InvalidState;
    }
    m_impl->currentState.store(StressTestState::Running);
    auto startTime = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(m_impl->mtx);
        for (auto& dev : m_impl->devices) {
            dev.connected = true;
        }
    }

    for (auto& dev : m_impl->devices) {
        if (m_impl->currentState.load() != StressTestState::Running) break;
        if (probabilityCheck(m_impl->config.randomFailureRate) &&
            m_impl->currentFailure.load() == FailureMode::RandomDisconnect) {
            m_impl->failedOps.fetch_add(m_impl->config.operationsPerDevice);
            m_impl->recoveredOps.fetch_add(1);
            continue;
        }
        m_impl->runOperationsOnDevice(dev);
    }

    auto endTime = std::chrono::steady_clock::now();
    m_impl->durationMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    if (m_impl->currentState.load() == StressTestState::Running) {
        m_impl->currentState.store(StressTestState::Completed);
    }
    return {};
}

Result<void> VirtualStressEnvironment::runParallelSessions(uint32_t count) {
    if (m_impl->currentState.load() != StressTestState::Idle &&
        m_impl->currentState.load() != StressTestState::Completed) {
        return ErrorCode::InvalidState;
    }
    m_impl->currentState.store(StressTestState::Running);
    auto startTime = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    uint32_t actualCount = std::min(count, static_cast<uint32_t>(m_impl->devices.size()));
    for (uint32_t i = 0; i < actualCount; ++i) {
        threads.emplace_back([this, i]() {
            if (m_impl->currentState.load() != StressTestState::Running) return;
            std::lock_guard<std::mutex> lock(m_impl->mtx);
            auto& dev = m_impl->devices[i];
            dev.connected = true;
            for (uint32_t op = 0; op < m_impl->config.operationsPerDevice; ++op) {
                if (m_impl->currentState.load() != StressTestState::Running) return;
                m_impl->totalOps.fetch_add(1);
                m_impl->simulateOperation(dev, op);
            }
            dev.operationsCompleted = m_impl->config.operationsPerDevice;
        });
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    m_impl->durationMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    if (m_impl->currentState.load() == StressTestState::Running) {
        m_impl->currentState.store(StressTestState::Completed);
    }
    return {};
}

Result<void> VirtualStressEnvironment::runConnectDisconnectCycles(uint32_t cycles) {
    if (m_impl->currentState.load() != StressTestState::Idle &&
        m_impl->currentState.load() != StressTestState::Completed) {
        return ErrorCode::InvalidState;
    }
    m_impl->currentState.store(StressTestState::Running);
    auto startTime = std::chrono::steady_clock::now();

    for (uint32_t c = 0; c < cycles; ++c) {
        if (m_impl->currentState.load() != StressTestState::Running) break;
        for (auto& dev : m_impl->devices) {
            dev.connected = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            dev.connected = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            m_impl->totalOps.fetch_add(2);
            m_impl->successOps.fetch_add(2);
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    m_impl->durationMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    if (m_impl->currentState.load() == StressTestState::Running) {
        m_impl->currentState.store(StressTestState::Completed);
    }
    return {};
}

Result<void> VirtualStressEnvironment::injectFailure(FailureMode mode) {
    m_impl->currentFailure.store(mode);
    return {};
}

Result<void> VirtualStressEnvironment::simulateHotplug() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    VirtualDeviceConfig cfg;
    cfg.deviceId = "HotplugDevice_" + std::to_string(m_impl->devices.size());
    cfg.type = VirtualDeviceType::Generic;
    cfg.connectTimeMs = 5;
    m_impl->devices.push_back({cfg, true, 0});
    m_impl->totalOps.fetch_add(1);
    m_impl->successOps.fetch_add(1);
    return {};
}

Result<void> VirtualStressEnvironment::simulatePowerLoss() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    for (auto& dev : m_impl->devices) {
        dev.connected = false;
        dev.operationsCompleted = 0;
    }
    m_impl->totalOps.fetch_add(1);
    m_impl->failedOps.fetch_add(1);
    m_impl->recoveredOps.fetch_add(1);
    return {};
}

Result<void> VirtualStressEnvironment::simulateTransportCorruption() {
    m_impl->currentFailure.store(FailureMode::TransportCorruption);
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->totalOps.fetch_add(1);
    m_impl->failedOps.fetch_add(1);
    m_impl->errorList.push_back("Transport corruption injected");
    return {};
}

Result<void> VirtualStressEnvironment::cancel() {
    m_impl->currentState.store(StressTestState::Cancelled);
    return {};
}

Result<StressTestResult> VirtualStressEnvironment::result() const {
    StressTestResult r;
    r.totalOperations = m_impl->totalOps.load();
    r.succeeded = m_impl->successOps.load();
    r.failed = m_impl->failedOps.load();
    r.timedOut = m_impl->timeoutOps.load();
    r.recovered = m_impl->recoveredOps.load();
    r.totalDurationMs = m_impl->durationMs.load();
    r.state = m_impl->currentState.load();
    {
        std::lock_guard<std::mutex> lock(m_impl->mtx);
        r.errors = m_impl->errorList;
    }
    return r;
}

StressTestState VirtualStressEnvironment::state() const {
    return m_impl->currentState.load();
}

} }
