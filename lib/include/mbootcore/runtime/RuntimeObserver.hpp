#pragma once

#include <mbootcore/runtime/RuntimeEvents.hpp>
#include <mbootcore/runtime/RuntimeStatistics.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace mbootcore {
namespace runtime {

class Runtime;

class RuntimeObserver {
public:
    virtual ~RuntimeObserver() = default;
    virtual void onRuntimeEvent(const RuntimeEvent& event) = 0;
    virtual void onStatisticsUpdated(const RuntimeStatistics& stats) = 0;
    virtual void onHealthChanged(const RuntimeHealth& health) = 0;
};

class ObserverManager {
public:
    void addObserver(RuntimeObserver* observer);
    void addObserver(std::shared_ptr<RuntimeObserver> observer);
    void removeObserver(RuntimeObserver* observer);
    void notifyEvent(const RuntimeEvent& event);
    void notifyStatistics(const RuntimeStatistics& stats);
    void notifyHealth(const RuntimeHealth& health);

private:
    std::vector<std::shared_ptr<RuntimeObserver>> copyObservers() const;
    std::vector<std::shared_ptr<RuntimeObserver>> m_observers;
    mutable std::mutex m_mutex;
};

} // namespace runtime
} // namespace mbootcore
