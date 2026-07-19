#pragma once

#include <mbootcore/pipeline/PipelineStage.hpp>
#include <mbootcore/domain/Error.hpp>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>

namespace mbootcore {
namespace pipeline {

enum class RecoveryAction : uint8_t {
    Retry,
    Rollback,
    RestartStage,
    Abort
};

struct RecoveryRule {
    PipelineStage stage{PipelineStage::Disconnected};
    int maxRetries{2};
    RecoveryAction action{RecoveryAction::Retry};
    RecoveryAction onExhaustedAction{RecoveryAction::Abort};
};

constexpr std::string_view toString(RecoveryAction action) {
    switch (action) {
        case RecoveryAction::Retry:       return "Retry";
        case RecoveryAction::Rollback:    return "Rollback";
        case RecoveryAction::RestartStage: return "RestartStage";
        case RecoveryAction::Abort:       return "Abort";
    }
    return "Unknown";
}

struct RecoveryContext {
    PipelineStage failedStage{PipelineStage::Disconnected};
    PipelineStage rollbackTarget{PipelineStage::Disconnected};
    ErrorCode error{ErrorCode::Success};
    int retryCount{0};
    std::string failureDescription;

    RecoveryContext() = default;
    RecoveryContext(PipelineStage stage, ErrorCode err, std::string desc)
        : failedStage(stage)
        , rollbackTarget(stage)
        , error(err)
        , failureDescription(std::move(desc)) {}
};

class RecoveryStrategy {
public:
    using RecoveryDecision = std::function<RecoveryAction(const RecoveryContext& context)>;

    RecoveryStrategy() = default;

    void setDefaultRule(RecoveryAction action) { m_defaultAction = action; }
    void setRuleForStage(PipelineStage stage, const RecoveryRule& rule) {
        m_stageRules[stage] = rule;
    }
    void setCustomDecision(PipelineStage stage, RecoveryDecision decision) {
        m_customDecisions[stage] = std::move(decision);
    }

    [[nodiscard]] RecoveryAction decide(const RecoveryContext& context) const {
        auto customIt = m_customDecisions.find(context.failedStage);
        if (customIt != m_customDecisions.end() && customIt->second) {
            return customIt->second(context);
        }

        auto ruleIt = m_stageRules.find(context.failedStage);
        if (ruleIt != m_stageRules.end()) {
            if (context.retryCount >= ruleIt->second.maxRetries) {
                return ruleIt->second.onExhaustedAction;
            }
            return RecoveryAction::Retry;
        }

        if (context.retryCount >= 2) {
            return m_defaultAction;
        }
        return RecoveryAction::Retry;
    }

    PipelineStage rollbackTarget(PipelineStage failedStage, int levels) const {
        auto it = m_rollbackMap.find(failedStage);
        if (it != m_rollbackMap.end()) {
            return it->second;
        }
        int current = static_cast<int>(failedStage);
        int target = std::max(0, current - levels);
        if (target < static_cast<int>(PipelineStage::Disconnected)) {
            target = static_cast<int>(PipelineStage::Disconnected);
        }
        return static_cast<PipelineStage>(target);
    }

    void setRollbackTarget(PipelineStage from, PipelineStage to) {
        m_rollbackMap[from] = to;
    }

private:
    RecoveryAction m_defaultAction{RecoveryAction::Abort};
    std::unordered_map<PipelineStage, RecoveryRule> m_stageRules;
    std::unordered_map<PipelineStage, RecoveryDecision> m_customDecisions;
    std::unordered_map<PipelineStage, PipelineStage> m_rollbackMap;
};

} // namespace pipeline
} // namespace mbootcore
