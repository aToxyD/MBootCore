#include <mbootcore/job/RecoveryPolicies.hpp>

namespace mbootcore {
namespace job {

RecoveryAction DefaultRecoveryPolicy::evaluate(const std::string&, ErrorCode error,
                                                uint32_t attemptCount) {
    if (error == ErrorCode::Cancelled) {
        return RecoveryAction::Abort;
    }

    if (attemptCount < static_cast<uint32_t>(m_maxRetries)) {
        return RecoveryAction::Retry;
    }

    if (m_canRollback && attemptCount >= static_cast<uint32_t>(m_maxRetries)) {
        if (m_rollbackCount < m_maxRollbacks) {
            m_rollbackCount++;
            return RecoveryAction::Rollback;
        }
    }

    return RecoveryAction::Abort;
}

RecoveryAction RollbackOnFailurePolicy::evaluate(const std::string&, ErrorCode error,
                                                  uint32_t) {
    if (error == ErrorCode::Cancelled) {
        return RecoveryAction::Abort;
    }

    if (m_rollbackCount < m_maxRollbacks) {
        m_rollbackCount++;
        return RecoveryAction::Rollback;
    }

    return RecoveryAction::Abort;
}

} // namespace job
} // namespace mbootcore
