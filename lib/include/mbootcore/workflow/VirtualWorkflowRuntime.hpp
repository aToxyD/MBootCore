#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/WorkflowBuilder.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/VirtualFirmware.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/domain/Types.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>

namespace mbootcore {
namespace workflow {

struct VirtualWorkflowConfig {
    bool simulateDisconnect{false};
    bool simulateTimeout{false};
    bool simulateLoaderFailure{false};
    bool simulateVerificationFailure{false};
    bool simulateCorruptFirmware{false};
    bool simulateCancel{false};
    bool simulateRetryScenario{false};
    bool simulateRollbackScenario{false};

    std::chrono::milliseconds stepDelay{10};
    std::chrono::milliseconds disconnectAfter{0};
    std::chrono::milliseconds timeoutAfter{0};

    size_t failOnStepIndex{std::numeric_limits<size_t>::max()};
    size_t cancelOnStepIndex{std::numeric_limits<size_t>::max()};
};

class VirtualStep : public IWorkflowStep {
public:
    VirtualStep(const std::string& name, WorkflowStepType type,
                ErrorCode error = ErrorCode::Success);

    Result<void> prepare(WorkflowContext& context) override;
    Result<void> execute(WorkflowContext& context) override;
    Result<void> rollback(WorkflowContext& context) override;

    std::string name() const noexcept override { return m_name; }
    WorkflowStepType type() const noexcept override { return m_type; }
    double progress() const noexcept override { return m_progress; }

    void setError(ErrorCode error) { m_executeError = error; }
    void setRollbackError(ErrorCode error) { m_rollbackError = error; }
    void setProgress(double p) { m_progress = p; }

    bool wasPrepared() const noexcept { return m_prepared; }
    bool wasExecuted() const noexcept { return m_executed; }
    bool wasRolledBack() const noexcept { return m_rolledBack; }

private:
    std::string m_name;
    WorkflowStepType m_type;
    ErrorCode m_executeError{ErrorCode::Success};
    ErrorCode m_rollbackError{ErrorCode::Success};
    double m_progress{0.0};
    bool m_prepared{false};
    bool m_executed{false};
    bool m_rolledBack{false};
};

class VirtualWorkflowRuntime {
public:
    VirtualWorkflowRuntime();

    void setConfig(const VirtualWorkflowConfig& config) { m_config = config; }
    const VirtualWorkflowConfig& config() const noexcept { return m_config; }
    VirtualWorkflowConfig& config() noexcept { return m_config; }

    std::unique_ptr<firmware::FirmwarePackage> createFirmwarePackage();
    std::unique_ptr<WorkflowEngine> createWorkflow();
    WorkflowContext createContext();

    VirtualStep* createStep(const std::string& name,
                            WorkflowStepType type);
    VirtualStep* addStepToWorkflow(WorkflowEngine& engine,
                                   const std::string& name,
                                   WorkflowStepType type);

    size_t buildFlashWorkflow(WorkflowEngine& engine);
    size_t buildBackupWorkflow(WorkflowEngine& engine,
                                const std::string& partition = {});
    size_t buildRestoreWorkflow(WorkflowEngine& engine,
                                 const std::string& partition = {});
    size_t buildRecoveryWorkflow(WorkflowEngine& engine);
    size_t buildCancelWorkflow(WorkflowEngine& engine);
    size_t buildRollbackWorkflow(WorkflowEngine& engine);
    size_t buildLargeWorkflow(WorkflowEngine& engine, size_t repeatCount = 10);

    // Callbacks
    using RuntimeCallback = std::function<void(const std::string& event)>;
    void setCallback(RuntimeCallback cb) { m_callback = std::move(cb); }

    // Access steps
    VirtualStep* getStep(const std::string& name) const;
    size_t createdStepCount() const noexcept { return m_steps.size(); }

    // Reset
    void reset();

private:
    VirtualWorkflowConfig m_config;
    std::unordered_map<std::string, VirtualStep*> m_steps;
    std::vector<std::unique_ptr<VirtualStep>> m_ownedSteps;
    firmware::VirtualPackageGenerator m_packageGen;
    RuntimeCallback m_callback;

    void emitEvent(const std::string& event);
    std::string makeStepName(WorkflowStepType type);
};

} // namespace workflow
} // namespace mbootcore
