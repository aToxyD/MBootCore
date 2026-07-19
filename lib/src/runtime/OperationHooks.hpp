#pragma once

#include <mbootcore/runtime/OrchestrationTypes.hpp>
#include <mbootcore/runtime/RuntimeEvents.hpp>
#include <mbootcore/runtime/RuntimeStatistics.hpp>
#include <mbootcore/runtime/RuntimeObserver.hpp>

#include <mutex>

namespace mbootcore {
namespace runtime {

struct RuntimeState;

class OperationHooks {
public:
    OperationHooks(RuntimeState& state, ObserverManager& observers) noexcept
        : m_state(state), m_observers(observers) {}

    void onSuccess(Operation op) {
        auto now = std::chrono::steady_clock::now();

        updateStat(op);
        auto ev = makeEvent(eventTypeFor(op, true), messageFor(op, true), true, ErrorCode::Success);
        ev.timestamp = now;
        fireEvent(ev);
    }

    void onFailure(Operation op, ErrorCode error) {
        auto now = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(m_state.statsMutex);
            ++m_state.stats.totalErrors;
        }

        auto ev = makeEvent(eventTypeFor(op, false), messageFor(op, false), false, error);
        ev.timestamp = now;
        fireEvent(ev);
    }

    void onCancel(Operation op) {
        (void)op;
        auto now = std::chrono::steady_clock::now();

        auto ev = makeEvent(RuntimeEventType::RuntimeStopped, "Operation cancelled", false, ErrorCode::Cancelled);
        ev.timestamp = now;
        fireEvent(ev);
    }

private:
    RuntimeEvent makeEvent(RuntimeEventType type, const std::string& message,
                           bool success, ErrorCode error) const {
        RuntimeEvent ev;
        ev.type = type;
        ev.message = message;
        ev.success = success;
        ev.error = error;
        ev.timestamp = std::chrono::steady_clock::now();
        return ev;
    }

    void fireEvent(const RuntimeEvent& event) {
        m_observers.notifyEvent(event);
        RuntimeStatistics snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state.statsMutex);
            auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::steady_clock::now() - m_state.startTime);
            m_state.stats.uptimeSeconds = elapsed.count();
            snapshot = m_state.stats;
        }
        m_observers.notifyStatistics(snapshot);
    }

    void updateStat(Operation op) {
        std::lock_guard<std::mutex> lock(m_state.statsMutex);
        switch (op) {
            case Operation::Connect:           ++m_state.stats.devicesConnected; break;
            case Operation::Disconnect:        ++m_state.stats.devicesDisconnected;
                                               ++m_state.stats.totalDisconnects; break;
            case Operation::LoadFirmwarePackage: ++m_state.stats.packagesInstalled; break;
            case Operation::ExecuteWorkflow:   ++m_state.stats.workflowsExecuted; break;
            case Operation::RunJob:
            case Operation::RunJobs:           ++m_state.stats.jobsExecuted; break;
            case Operation::InstallPlugin:     ++m_state.stats.pluginsLoaded; break;
            case Operation::RegisterVendor:    ++m_state.stats.vendorsLoaded; break;
            default: break;
        }
    }

    static RuntimeEventType eventTypeFor(Operation op, bool success) noexcept {
        if (!success) {
            if (op == Operation::ExecuteWorkflow) return RuntimeEventType::WorkflowFailed;
            return RuntimeEventType::RuntimeStopped;
        }
        switch (op) {
            case Operation::Connect:            return RuntimeEventType::DeviceConnected;
            case Operation::Disconnect:         return RuntimeEventType::DeviceDisconnected;
            case Operation::LoadFirmwarePackage: return RuntimeEventType::PackageLoaded;
            case Operation::ExecuteWorkflow:    return RuntimeEventType::WorkflowFinished;
            case Operation::RunJob:             return RuntimeEventType::JobStarted;
            case Operation::InstallPlugin:      return RuntimeEventType::PluginLoaded;
            case Operation::RegisterVendor:     return RuntimeEventType::VendorLoaded;
            default:                            return RuntimeEventType::RuntimeStarted;
        }
    }

    static std::string messageFor(Operation op, bool success) noexcept {
        if (!success) return defaultFailureMessage(op);
        switch (op) {
            case Operation::Connect:            return "Device connected";
            case Operation::Disconnect:         return "Device disconnected";
            case Operation::LoadFirmwarePackage: return "Firmware package loaded";
            case Operation::ExecuteWorkflow:    return "Workflow completed";
            case Operation::RunJob:             return "Job scheduled";
            case Operation::InstallPlugin:      return "Plugin installed";
            case Operation::RegisterVendor:     return "Vendor registered";
            default:                            return "Operation completed";
        }
    }

    static std::string defaultFailureMessage(Operation op) noexcept {
        switch (op) {
            case Operation::Connect:            return "Connection failed";
            case Operation::Disconnect:         return "Disconnection failed";
            case Operation::ExecuteWorkflow:    return "Workflow failed";
            default:                            return "Operation failed";
        }
    }

    RuntimeState& m_state;
    ObserverManager& m_observers;
};

} // namespace runtime
} // namespace mbootcore
