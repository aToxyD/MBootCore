#pragma once

#include <memory>
#include <string>
#include <mbootcore/faults/FaultTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace faults {

class FaultInjector {
public:
    FaultInjector();
    ~FaultInjector();
    FaultInjector(const FaultInjector&) = delete;
    FaultInjector& operator=(const FaultInjector&) = delete;
    FaultInjector(FaultInjector&&) noexcept;
    FaultInjector& operator=(FaultInjector&&) noexcept;

    Result<void> configure(const FaultConfig& config);
    Result<FaultConfig> config() const;
    Result<bool> shouldInject() const;
    Result<FaultEvent> inject(const std::string& context = "");
    Result<void> reset();
    FaultState state() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class FaultManager {
public:
    FaultManager();
    ~FaultManager();
    FaultManager(const FaultManager&) = delete;
    FaultManager& operator=(const FaultManager&) = delete;
    FaultManager(FaultManager&&) noexcept;
    FaultManager& operator=(FaultManager&&) noexcept;

    Result<void> registerInjector(const std::string& name, std::unique_ptr<FaultInjector> injector);
    Result<void> unregisterInjector(const std::string& name);
    Result<void> injectFault(FaultType type, const std::string& context = "");
    Result<std::vector<FaultEvent>> events() const;
    Result<void> clearEvents();
    Result<void> resetAll();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
