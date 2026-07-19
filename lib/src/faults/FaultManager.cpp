#include <mbootcore/faults/FaultInjector.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace mbootcore { namespace faults {

struct FaultManager::Impl {
    std::unordered_map<std::string, std::unique_ptr<FaultInjector>> injectors;
    std::vector<FaultEvent> allEvents;
    mutable std::mutex mtx;
};

FaultManager::FaultManager()
    : m_impl(std::make_unique<Impl>()) {}

FaultManager::~FaultManager() = default;
FaultManager::FaultManager(FaultManager&&) noexcept = default;
FaultManager& FaultManager::operator=(FaultManager&&) noexcept = default;

Result<void> FaultManager::registerInjector(const std::string& name, std::unique_ptr<FaultInjector> injector) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    if (m_impl->injectors.find(name) != m_impl->injectors.end()) {
        return ErrorCode::AlreadyExists;
    }
    m_impl->injectors.emplace(name, std::move(injector));
    return {};
}

Result<void> FaultManager::unregisterInjector(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    auto it = m_impl->injectors.find(name);
    if (it == m_impl->injectors.end()) {
        return ErrorCode::PluginNotFound;
    }
    m_impl->injectors.erase(it);
    return {};
}

Result<void> FaultManager::injectFault(FaultType type, const std::string& context) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    for (auto& [name, injector] : m_impl->injectors) {
        auto cfg = injector->config();
        if (!cfg.isOk()) continue;
        if (cfg.value().type == type) {
            auto should = injector->shouldInject();
            if (should.isOk() && should.value()) {
                auto result = injector->inject(context);
                if (result.isOk()) {
                    m_impl->allEvents.push_back(std::move(result.value()));
                    return {};
                }
            }
        }
    }
    return {};
}

Result<std::vector<FaultEvent>> FaultManager::events() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->allEvents;
}

Result<void> FaultManager::clearEvents() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->allEvents.clear();
    return {};
}

Result<void> FaultManager::resetAll() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    for (auto& [name, injector] : m_impl->injectors) {
        (void)injector->reset();
    }
    m_impl->allEvents.clear();
    return {};
}

} }
