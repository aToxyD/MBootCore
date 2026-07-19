#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <mbootcore/workflow/WorkflowRecovery.hpp>
#include <mbootcore/workflow/WorkflowProgressEngine.hpp>
#include <mbootcore/workflow/WorkflowHistory.hpp>

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <functional>

namespace mbootcore {
namespace workflow {

class WorkflowEngine : public IWorkflow {
public:
    WorkflowEngine();
    ~WorkflowEngine() override;

    WorkflowEngine(const WorkflowEngine&) = delete;
    WorkflowEngine& operator=(const WorkflowEngine&) = delete;
    WorkflowEngine(WorkflowEngine&&) = delete;
    WorkflowEngine& operator=(WorkflowEngine&&) = delete;

    // IWorkflow
    Result<void> prepare() override;
    Result<void> run() override;
    Result<void> pause() override;
    Result<void> resume() override;
    void cancel() noexcept override;
    Result<void> rollback() override;
    Result<void> reset() override;

    WorkflowProgress progress() const noexcept override;
    WorkflowResult result() const noexcept override;
    WorkflowStatistics statistics() const noexcept override;
    std::vector<std::string> history() const override;

    WorkflowState state() const noexcept override { return m_state; }
    std::string stateString() const override;

    void setContext(WorkflowContext context) override { m_context = std::move(context); }
    WorkflowContext& context() noexcept override { return m_context; }
    const WorkflowContext& context() const noexcept override { return m_context; }

    void setOptions(const WorkflowOptions& options) override { m_options = options; }
    const WorkflowOptions& options() const noexcept override { return m_options; }

    void addStep(std::unique_ptr<IWorkflowStep> step);
    void insertStep(size_t index, std::unique_ptr<IWorkflowStep> step);
    void removeStep(const std::string& stepName);
    void clearSteps();
    size_t stepCount() const noexcept { return m_steps.size(); }
    IWorkflowStep* findStep(const std::string& name);
    const IWorkflowStep* findStep(const std::string& name) const;

    // Callbacks
    using StateCallback = std::function<void(WorkflowState oldState, WorkflowState newState)>;
    using ErrorCallback = std::function<void(ErrorCode error, const std::string& message)>;
    using ProgressCallback = std::function<void(const WorkflowProgress& progress)>;

    void setStateCallback(StateCallback cb) { m_stateCb = std::move(cb); }
    void setErrorCallback(ErrorCallback cb) { m_errorCb = std::move(cb); }
    void setProgressCallback(ProgressCallback cb) { m_progressCb = std::move(cb); }

    WorkflowRecovery& recovery() noexcept { return m_recovery; }
    const WorkflowRecovery& recovery() const noexcept { return m_recovery; }

    WorkflowProgressEngine& progressEngine() noexcept { return m_progressEngine; }
    const WorkflowProgressEngine& progressEngine() const noexcept { return m_progressEngine; }

    WorkflowHistory& workflowHistory() noexcept { return m_history; }
    const WorkflowHistory& workflowHistory() const noexcept { return m_history; }

private:
    Result<void> executeStep(size_t index);
    Result<void> performRecovery(size_t index, const ErrorCode& error);
    void setState(WorkflowState newState);
    void notifyStateChange(WorkflowState old, WorkflowState current);
    void notifyError(ErrorCode error, const std::string& msg);
    void notifyProgress();

    bool checkCancelled() const noexcept {
        return m_cancelled.load() || m_context.isCancelled();
    }

    struct StepEntry {
        std::unique_ptr<IWorkflowStep> step;
        bool completed{false};
        bool failed{false};
    };

    std::vector<StepEntry> m_steps;
    WorkflowContext m_context;
    WorkflowOptions m_options;
    WorkflowRecovery m_recovery;
    WorkflowProgressEngine m_progressEngine;
    WorkflowHistory m_history;

    std::atomic<WorkflowState> m_state{WorkflowState::Created};
    std::atomic<bool> m_cancelled{false};
    std::atomic<bool> m_paused{false};
    size_t m_currentStepIndex{0};

    WorkflowResult m_result;
    WorkflowStatistics m_stats;

    StateCallback m_stateCb;
    ErrorCallback m_errorCb;
    ProgressCallback m_progressCb;
};

} // namespace workflow
} // namespace mbootcore
