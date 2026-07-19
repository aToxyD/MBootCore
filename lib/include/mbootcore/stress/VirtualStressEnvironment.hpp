#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mbootcore/stress/StressTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace stress {

class VirtualStressEnvironment {
public:
    VirtualStressEnvironment();
    ~VirtualStressEnvironment();
    VirtualStressEnvironment(const VirtualStressEnvironment&) = delete;
    VirtualStressEnvironment& operator=(const VirtualStressEnvironment&) = delete;
    VirtualStressEnvironment(VirtualStressEnvironment&&) noexcept;
    VirtualStressEnvironment& operator=(VirtualStressEnvironment&&) noexcept;

    Result<void> initialize(const StressTestConfig& config);
    Result<void> createDevices(uint32_t count);
    Result<void> addDevice(const VirtualDeviceConfig& config);
    Result<void> runStressTest();
    Result<void> runParallelSessions(uint32_t count);
    Result<void> runConnectDisconnectCycles(uint32_t cycles);
    Result<void> injectFailure(FailureMode mode);
    Result<void> simulateHotplug();
    Result<void> simulatePowerLoss();
    Result<void> simulateTransportCorruption();
    Result<void> cancel();
    Result<StressTestResult> result() const;
    StressTestState state() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
