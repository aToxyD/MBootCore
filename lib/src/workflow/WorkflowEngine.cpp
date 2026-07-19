#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/workflow/WorkflowExecutor.hpp>

#include <thread>
#include <sstream>

namespace mbootcore {
namespace workflow {

WorkflowEngine::WorkflowEngine() = default;

WorkflowEngine::~WorkflowEngine() {
    cancel();
}

void WorkflowEngine::addStep(std::unique_ptr<IWorkflowStep> step) {
    StepEntry entry;
    entry.step = std::move(step);
    m_steps.push_back(std::move(entry));
}

void WorkflowEngine::insertStep(size_t index, std::unique_ptr<IWorkflowStep> step) {
    if (index > m_steps.size()) index = m_steps.size();
    StepEntry entry;
    entry.step = std::move(step);
    m_steps.insert(m_steps.begin() + static_cast<ptrdiff_t>(index), std::move(entry));
}

void WorkflowEngine::removeStep(const std::string& stepName) {
    for (auto it = m_steps.begin(); it != m_steps.end(); ++it) {
        if (it->step && it->step->name() == stepName) {
            m_steps.erase(it);
            return;
        }
    }
}

void WorkflowEngine::clearSteps() {
    m_steps.clear();
    m_currentStepIndex = 0;
    m_progressEngine.reset();
}

IWorkflowStep* WorkflowEngine::findStep(const std::string& name) {
    for (auto& entry : m_steps) {
        if (entry.step && entry.step->name() == name) return entry.step.get();
    }
    return nullptr;
}

const IWorkflowStep* WorkflowEngine::findStep(const std::string& name) const {
    for (const auto& entry : m_steps) {
        if (entry.step && entry.step->name() == name) return entry.step.get();
    }
    return nullptr;
}

Result<void> WorkflowEngine::prepare() {
    if (m_state != WorkflowState::Created) {
        return ErrorCode::InvalidState;
    }

    setState(WorkflowState::Ready);
    m_stats.startTime = std::chrono::steady_clock::now();
    m_progressEngine.setTotalSteps(m_steps.size());

    for (size_t i = 0; i < m_steps.size(); ++i) {
        auto& entry = m_steps[i];
        if (entry.step) {
            auto result = entry.step->prepare(m_context);
            if (result.isError()) {
                m_stats.errors++;
                setState(WorkflowState::Failed);
                m_result.success = false;
                m_result.error = result.error();
                m_result.message = "Failed to prepare step: " + entry.step->name();
                return result;
            }
        }
    }

    return {};
}

Result<void> WorkflowEngine::run() {
    if (m_state == WorkflowState::Created) {
        MBOOT_TRY(prepare());
    }

    if (m_state != WorkflowState::Ready && m_state != WorkflowState::Paused) {
        return ErrorCode::InvalidState;
    }

    if (m_steps.empty()) {
        setState(WorkflowState::Completed);
        m_result.success = true;
        m_result.error = ErrorCode::Success;
        return {};
    }

    m_cancelled = false;
    m_paused = false;
    setState(WorkflowState::Running);

    for (size_t i = m_currentStepIndex; i < m_steps.size(); ++i) {
        if (checkCancelled()) {
            setState(WorkflowState::Cancelled);
            m_result.success = false;
            m_result.error = ErrorCode::Cancelled;
            return ErrorCode::Cancelled;
        }

        // Handle pause
        if (m_paused) {
            setState(WorkflowState::Paused);
            while (m_paused && !checkCancelled()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (checkCancelled()) {
                setState(WorkflowState::Cancelled);
                m_result.success = false;
                m_result.error = ErrorCode::Cancelled;
                return ErrorCode::Cancelled;
            }
            setState(WorkflowState::Running);
        }

        m_currentStepIndex = i;
        auto result = executeStep(i);

        if (result.isError()) {
            if (result.error() == ErrorCode::Cancelled) {
                setState(WorkflowState::Cancelled);
                return result;
            }

            if (m_options.continueOnWarning) {
                ++m_stats.warnings;
                m_result.error = result.error();
                m_result.message = "Warning: step failed but continuing";
                continue;
            }

            setState(WorkflowState::Failed);
            return result;
        }
    }

    m_stats.finishTime = std::chrono::steady_clock::now();
    m_stats.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        m_stats.finishTime - m_stats.startTime);

    if (m_stats.warnings > 0 && m_options.continueOnWarning) {
        m_result.success = false;
        m_result.statistics = m_stats;
        setState(WorkflowState::Failed);
        return m_result.error != ErrorCode::Success
            ? m_result.error : ErrorCode::WorkflowStepFailed;
    }

    setState(WorkflowState::Completed);
    m_result.success = true;
    m_result.statistics = m_stats;

    return {};
}

Result<void> WorkflowEngine::pause() {
    if (m_state != WorkflowState::Running) {
        return ErrorCode::InvalidState;
    }
    m_paused = true;
    setState(WorkflowState::Paused);
    return {};
}

Result<void> WorkflowEngine::resume() {
    if (m_state != WorkflowState::Paused) {
        return ErrorCode::InvalidState;
    }
    m_paused = false;
    setState(WorkflowState::Running);
    return {};
}

void WorkflowEngine::cancel() noexcept {
    m_cancelled = true;
    setState(WorkflowState::Cancelled);
    m_progressEngine.markCancelled();
}

Result<void> WorkflowEngine::rollback() {
    if (m_state != WorkflowState::Failed && m_state != WorkflowState::Cancelled) {
        return ErrorCode::InvalidState;
    }

    setState(WorkflowState::RollingBack);
    ErrorCode lastError = ErrorCode::Success;

    for (size_t i = m_currentStepIndex; i > 0; --i) {
        auto& entry = m_steps[i - 1];
        if (entry.step && entry.completed) {
            auto rbResult = entry.step->rollback(m_context);
            if (rbResult.isError()) {
                lastError = rbResult.error();
                ++m_stats.errors;
            } else {
                ++m_stats.rollbacks;
            }
        }
    }

    if (lastError != ErrorCode::Success) {
        m_result.success = false;
        m_result.error = lastError;
        return lastError;
    }

    setState(WorkflowState::Ready);
    return {};
}

Result<void> WorkflowEngine::reset() {
    cancel();
    m_steps.clear();
    m_currentStepIndex = 0;
    m_cancelled = false;
    m_paused = false;
    m_stats = WorkflowStatistics{};
    m_result = WorkflowResult{};
    m_progressEngine.reset();
    m_history.clear();
    m_recovery.clearCustomRecoveries();
    setState(WorkflowState::Created);
    return {};
}

WorkflowProgress WorkflowEngine::progress() const noexcept {
    return m_progressEngine.currentProgress();
}

WorkflowResult WorkflowEngine::result() const noexcept {
    return m_result;
}

WorkflowStatistics WorkflowEngine::statistics() const noexcept {
    return m_stats;
}

std::vector<std::string> WorkflowEngine::history() const {
    auto entries = m_history.recent();
    std::vector<std::string> strings;
    strings.reserve(entries.size());
    for (const auto& e : entries) {
        strings.push_back(e.workflowId + ":" + (e.success ? "OK" : "FAIL"));
    }
    return strings;
}

std::string WorkflowEngine::stateString() const {
    switch (m_state.load()) {
        case WorkflowState::Created:     return "Created";
        case WorkflowState::Ready:       return "Ready";
        case WorkflowState::Running:     return "Running";
        case WorkflowState::Paused:      return "Paused";
        case WorkflowState::Cancelled:   return "Cancelled";
        case WorkflowState::Completed:   return "Completed";
        case WorkflowState::Failed:      return "Failed";
        case WorkflowState::RollingBack: return "RollingBack";
    }
    return "Unknown";
}

Result<void> WorkflowEngine::executeStep(size_t index) {
    auto& entry = m_steps[index];
    if (!entry.step) {
        return ErrorCode::InvalidArgument;
    }

    auto* step = entry.step.get();
    m_progressEngine.setCurrentStep(step->name(), step->type());

    int retryCount = 0;
    const int maxRetries = m_options.retryCount > 0 ? m_options.retryCount : 1;

    while (retryCount < maxRetries) {
        if (checkCancelled()) {
            return ErrorCode::Cancelled;
        }

        auto execResult = step->execute(m_context);

        if (execResult) {
            entry.completed = true;
            m_progressEngine.markStepCompleted();
            notifyProgress();

            WorkflowHistoryEntry histEntry;
            histEntry.workflowId = step->name();
            histEntry.success = true;
            histEntry.duration = std::chrono::milliseconds(0);
            histEntry.finalState = WorkflowState::Completed;
            m_history.addEntry(histEntry);

            return {};
        }

        if (execResult.error() == ErrorCode::Cancelled) {
            entry.failed = true;
            m_progressEngine.markStepFailed("Cancelled");
            return execResult;
        }

        ++retryCount;
        ++m_stats.retries;

        auto decision = m_recovery.decide(step->type(), step->name(),
                                          execResult.error());

        switch (decision.action) {
            case RecoveryAction::Retry:
                if (decision.retryDelayMs > 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(decision.retryDelayMs));
                }
                continue;

            case RecoveryAction::Rollback: {
                auto rbResult = rollback();
                if (rbResult.isError()) {
                    ++m_stats.rollbacks;
                    entry.failed = true;
                    m_progressEngine.markStepFailed("Rollback failed");
                    notifyError(execResult.error(),
                                "Step failed and rollback failed: " + step->name());
                    return execResult;
                }
                entry.failed = true;
                return execResult;
            }

            case RecoveryAction::Skip:
                entry.completed = true;
                m_progressEngine.markStepCompleted();
                ++m_stats.warnings;
                return {};

            case RecoveryAction::Abort:
            default:
                entry.failed = true;
                m_progressEngine.markStepFailed(step->name());

                {
                    WorkflowHistoryEntry histEntry;
                    histEntry.workflowId = step->name();
                    histEntry.success = false;
                    histEntry.errorCode = execResult.error();
                    histEntry.errors.push_back(
                        std::string(toString(execResult.error())));
                    histEntry.finalState = WorkflowState::Failed;
                    m_history.addEntry(histEntry);
                }

                ++m_stats.errors;
                notifyError(execResult.error(),
                            "Step failed: " + step->name());
                return execResult;
        }
    }

    entry.failed = true;
    ++m_stats.errors;
    notifyError(ErrorCode::WorkflowStepFailed,
                "Step exhausted retries: " + entry.step->name());
    return ErrorCode::WorkflowStepFailed;
}

Result<void> WorkflowEngine::performRecovery(size_t index, const ErrorCode& error) {
    auto& entry = m_steps[index];
    if (!entry.step) {
        return ErrorCode::InvalidArgument;
    }

    auto decision = m_recovery.decide(entry.step->type(),
                                      entry.step->name(), error);

    switch (decision.action) {
        case RecoveryAction::Retry:
            return executeStep(index);
        case RecoveryAction::Skip:
            entry.completed = true;
            return {};
        case RecoveryAction::Rollback:
            return rollback();
        case RecoveryAction::Abort:
        default:
            return error;
    }
}

void WorkflowEngine::setState(WorkflowState newState) {
    auto oldState = m_state.exchange(newState);
    if (oldState != newState) {
        notifyStateChange(oldState, newState);
    }
}

void WorkflowEngine::notifyStateChange(WorkflowState old, WorkflowState current) {
    if (m_stateCb) {
        m_stateCb(old, current);
    }
}

void WorkflowEngine::notifyError(ErrorCode error, const std::string& msg) {
    if (m_errorCb) {
        m_errorCb(error, msg);
    }
}

void WorkflowEngine::notifyProgress() {
    if (m_progressCb) {
        m_progressCb(m_progressEngine.currentProgress());
    }
}

} // namespace workflow
} // namespace mbootcore
