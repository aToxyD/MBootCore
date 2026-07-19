#include <mbootcore/faults/FaultInjector.hpp>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>

namespace mbootcore { namespace faults {

namespace {

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

struct FaultInjector::Impl {
    FaultConfig config;
    std::atomic<uint32_t> triggerCount{0};
    std::atomic<FaultState> currentState{FaultState::Inactive};
    std::chrono::steady_clock::time_point activationTime;
    mutable std::mutex mtx;
};

FaultInjector::FaultInjector()
    : m_impl(std::make_unique<Impl>()) {}

FaultInjector::~FaultInjector() = default;
FaultInjector::FaultInjector(FaultInjector&&) noexcept = default;
FaultInjector& FaultInjector::operator=(FaultInjector&&) noexcept = default;

Result<void> FaultInjector::configure(const FaultConfig& config) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->config = config;
    m_impl->triggerCount.store(0);
    m_impl->currentState.store(config.enabled ? FaultState::Active : FaultState::Inactive);
    m_impl->activationTime = std::chrono::steady_clock::now();
    return {};
}

Result<FaultConfig> FaultInjector::config() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->config;
}

Result<bool> FaultInjector::shouldInject() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    if (!m_impl->config.enabled) return false;
    if (m_impl->currentState.load() == FaultState::Expired) return false;

    if (m_impl->config.scope == FaultScope::Once) {
        if (m_impl->triggerCount.load() >= 1) return false;
        return probabilityCheck(m_impl->config.probability);
    }

    if (m_impl->config.scope == FaultScope::Persistent) {
        if (m_impl->config.maxTriggers > 0 && m_impl->triggerCount.load() >= m_impl->config.maxTriggers) {
            return false;
        }
        return probabilityCheck(m_impl->config.probability);
    }

    if (m_impl->config.scope == FaultScope::Random) {
        if (m_impl->config.maxTriggers > 0 && m_impl->triggerCount.load() >= m_impl->config.maxTriggers) {
            return false;
        }
        return probabilityCheck(m_impl->config.probability);
    }

    if (m_impl->config.scope == FaultScope::Timed) {
        auto elapsed = std::chrono::steady_clock::now() - m_impl->activationTime;
        if (m_impl->config.duration.count() > 0 && elapsed >= m_impl->config.duration) {
            m_impl->currentState.store(FaultState::Expired);
            return false;
        }
        return probabilityCheck(m_impl->config.probability);
    }

    return false;
}

Result<FaultEvent> FaultInjector::inject(const std::string& context) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    if (m_impl->currentState.load() == FaultState::Expired) {
        return ErrorCode::InvalidState;
    }
    if (!m_impl->config.enabled) {
        return ErrorCode::InvalidState;
    }

    m_impl->triggerCount.fetch_add(1);

    FaultEvent event;
    event.type = m_impl->config.type;
    event.description = context.empty() ? "Fault injected" : context;
    event.timestamp = std::chrono::steady_clock::now();
    event.handled = false;

    if (m_impl->config.scope == FaultScope::Once) {
        m_impl->currentState.store(FaultState::Triggered);
    }

    if (m_impl->config.maxTriggers > 0 && m_impl->triggerCount.load() >= m_impl->config.maxTriggers) {
        m_impl->currentState.store(FaultState::Expired);
    }

    return event;
}

Result<void> FaultInjector::reset() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->triggerCount.store(0);
    m_impl->currentState.store(m_impl->config.enabled ? FaultState::Active : FaultState::Inactive);
    m_impl->activationTime = std::chrono::steady_clock::now();
    return {};
}

FaultState FaultInjector::state() const {
    return m_impl->currentState.load();
}

} }
