#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace mbootcore {
namespace workflow {

enum class RecoveryAction : uint32_t {
    Retry    = 0,
    Rollback = 1,
    Skip     = 2,
    Abort    = 3,
    Resume   = 4,
    Continue = 5
};

struct RecoveryDecision {
    RecoveryAction action{RecoveryAction::Abort};
    int retryDelayMs{0};
    int maxRetries{3};
    std::string reason;
};

class IRecoveryStrategy {
public:
    virtual ~IRecoveryStrategy() = default;

    virtual RecoveryDecision decide(WorkflowStepType stepType,
                                     const std::string& stepName,
                                     ErrorCode error,
                                     int retryCount) = 0;
    virtual std::string name() const noexcept = 0;
};

class DefaultRecoveryStrategy : public IRecoveryStrategy {
public:
    RecoveryDecision decide(WorkflowStepType stepType,
                             const std::string& stepName,
                             ErrorCode error,
                             int retryCount) override;
    std::string name() const noexcept override { return "Default"; }
};

class WorkflowRecovery {
public:
    WorkflowRecovery();

    RecoveryDecision decide(WorkflowStepType stepType,
                             const std::string& stepName,
                             ErrorCode error);

    void setStrategy(std::unique_ptr<IRecoveryStrategy> strategy);
    IRecoveryStrategy* strategy() const noexcept { return m_strategy.get(); }

    void setCustomRecovery(WorkflowStepType stepType,
                           std::unique_ptr<IRecoveryStrategy> strategy);
    IRecoveryStrategy* customRecovery(WorkflowStepType stepType) const;

    void clearCustomRecoveries();
    size_t strategyCount() const noexcept { return m_customStrategies.size() + (m_strategy ? 1 : 0); }

private:
    std::unique_ptr<IRecoveryStrategy> m_strategy;
    std::unordered_map<WorkflowStepType, std::unique_ptr<IRecoveryStrategy>> m_customStrategies;
};

} // namespace workflow
} // namespace mbootcore
