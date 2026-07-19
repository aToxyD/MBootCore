#include <mbootcore/workflow/WorkflowRecovery.hpp>

namespace mbootcore {
namespace workflow {

WorkflowRecovery::WorkflowRecovery() {
    m_strategy = std::make_unique<DefaultRecoveryStrategy>();
}

RecoveryDecision DefaultRecoveryStrategy::decide(WorkflowStepType stepType,
                                                   const std::string& stepName,
                                                   ErrorCode error,
                                                   int retryCount) {
    (void)stepName;
    RecoveryDecision decision;
    decision.maxRetries = 3;

    if (error == ErrorCode::Cancelled) {
        decision.action = RecoveryAction::Abort;
        decision.reason = "Operation cancelled";
        return decision;
    }

    switch (stepType) {
        case WorkflowStepType::Verify:
            decision.action = RecoveryAction::Abort;
            decision.reason = "Verification failed, cannot continue";
            return decision;

        case WorkflowStepType::Reboot:
        case WorkflowStepType::Disconnect:
            decision.action = RecoveryAction::Skip;
            decision.reason = "Non-critical step failed, skipping";
            return decision;

        default:
            break;
    }

    if (retryCount < decision.maxRetries) {
        decision.action = RecoveryAction::Retry;
        decision.retryDelayMs = 100 * (retryCount + 1);
        decision.reason = "Retrying after error";
        return decision;
    }

    switch (stepType) {
        case WorkflowStepType::Connect:
        case WorkflowStepType::Detect:
        case WorkflowStepType::Negotiate:
            decision.action = RecoveryAction::Retry;
            decision.retryDelayMs = 500;
            decision.reason = "Connection-related error, retrying";
            break;

        case WorkflowStepType::UploadLoader:
            decision.action = RecoveryAction::Retry;
            decision.reason = "Loader upload failed, retrying";
            break;

        case WorkflowStepType::Flash:
        case WorkflowStepType::GPT:
        case WorkflowStepType::Backup:
        case WorkflowStepType::Restore:
            if (retryCount < decision.maxRetries + 2) {
                decision.action = RecoveryAction::Rollback;
                decision.reason = "Storage operation failed, rolling back";
            } else {
                decision.action = RecoveryAction::Abort;
                decision.reason = "Max retries exceeded for storage operation";
            }
            break;

        default:
            decision.action = RecoveryAction::Abort;
            decision.reason = "Unknown step type, aborting";
            break;
    }

    return decision;
}

RecoveryDecision WorkflowRecovery::decide(WorkflowStepType stepType,
                                           const std::string& stepName,
                                           ErrorCode error) {
    auto it = m_customStrategies.find(stepType);
    if (it != m_customStrategies.end() && it->second) {
        return it->second->decide(stepType, stepName, error, 0);
    }

    if (m_strategy) {
        return m_strategy->decide(stepType, stepName, error, 0);
    }

    DefaultRecoveryStrategy fallback;
    return fallback.decide(stepType, stepName, error, 0);
}

void WorkflowRecovery::setStrategy(std::unique_ptr<IRecoveryStrategy> s) {
    m_strategy = std::move(s);
}

void WorkflowRecovery::setCustomRecovery(WorkflowStepType stepType,
                                          std::unique_ptr<IRecoveryStrategy> s) {
    if (s) {
        m_customStrategies[stepType] = std::move(s);
    }
}

IRecoveryStrategy* WorkflowRecovery::customRecovery(WorkflowStepType stepType) const {
    auto it = m_customStrategies.find(stepType);
    return it != m_customStrategies.end() ? it->second.get() : nullptr;
}

void WorkflowRecovery::clearCustomRecoveries() {
    m_customStrategies.clear();
}

} // namespace workflow
} // namespace mbootcore
