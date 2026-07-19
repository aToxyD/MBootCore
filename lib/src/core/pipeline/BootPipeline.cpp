#include <mbootcore/pipeline/BootPipeline.hpp>

namespace mbootcore {
namespace pipeline {

BootPipeline::BootPipeline()
    : m_recoveryStrategy(std::make_shared<RecoveryStrategy>()) {}

void BootPipeline::cancel() {
    m_cancelled = true;
    if (m_context.transport) {
        m_context.transport->cancel();
    }
}

Result<void> BootPipeline::run() {
    if (m_running) {
        return ErrorCode::AlreadyExists;
    }

    if (m_cancelled) {
        m_running = false;
        m_context.currentStage = PipelineStage::Cancelled;
        m_context.lastError = ErrorCode::Cancelled;
        updateProgress(PipelineStage::Cancelled);
        reportProgress();
        return ErrorCode::Cancelled;
    }

    m_running = true;
    m_cancelled = false;
    m_recoveryAttempts = 0;
    m_context.currentStage = PipelineStage::Disconnected;
    m_context.lastError = ErrorCode::Success;

    auto result = executeStage(PipelineStage::Connected);
    if (result.isError()) {
        m_running = false;
        return result;
    }

    m_running = false;
    if (m_context.currentStage == PipelineStage::Ready) {
        return {};
    }
    return m_context.lastError;
}

Result<void> BootPipeline::retry() {
    if (!isInTerminalState()) {
        return ErrorCode::InvalidArgument;
    }
    if (m_context.currentStage == PipelineStage::Error) {
        m_context.currentStage = PipelineStage::Disconnected;
    }
    m_cancelled = false;
    return run();
}

Result<void> BootPipeline::reset() {
    m_context.currentStage = PipelineStage::Disconnected;
    m_context.previousStage = PipelineStage::Disconnected;
    m_context.lastError = ErrorCode::Success;
    m_context.stageRetryCount = 0;
    m_stageRetryCounts.clear();
    m_cancelled = false;
    m_running = false;
    m_recoveryAttempts = 0;
    return {};
}

Result<void> BootPipeline::executeStage(PipelineStage stage) {
    m_context.currentStage = stage;
    updateProgress(stage);
    reportProgress();

    while (true) {
        if (m_cancelled) {
            m_context.currentStage = PipelineStage::Cancelled;
            m_context.lastError = ErrorCode::Cancelled;
            updateProgress(PipelineStage::Cancelled);
            reportProgress();
            return ErrorCode::Cancelled;
        }

        auto handlerIt = m_handlers.find(stage);
        if (handlerIt == m_handlers.end()) {
            auto next = nextStage(stage);
            if (next == PipelineStage::Ready || next == PipelineStage::Error) {
                m_context.currentStage = PipelineStage::Ready;
                updateProgress(PipelineStage::Ready);
                reportProgress();
                return {};
            }
            stage = next;
            m_context.currentStage = stage;
            updateProgress(stage);
            reportProgress();
            continue;
        }

        auto result = handlerIt->second(m_context);
        if (result.isOk()) {
            m_context.previousStage = stage;
            auto next = nextStage(stage);
            if (next == PipelineStage::Ready || next == PipelineStage::Error) {
                m_context.currentStage = PipelineStage::Ready;
                updateProgress(PipelineStage::Ready);
                reportProgress();
                return {};
            }
            stage = next;
            m_context.currentStage = stage;
            updateProgress(stage);
            reportProgress();
            continue;
        }

        auto failResult = handleStageFailure(stage, result.error());
        if (failResult.isError()) {
            return failResult;
        }

        stage = m_context.currentStage;
        updateProgress(stage);
        reportProgress();
    }
}

Result<void> BootPipeline::handleStageFailure(PipelineStage stage, const ErrorCode& error) {
    m_context.lastError = error;
    m_context.previousStage = m_context.currentStage;

    bool isCancel = (error == ErrorCode::Cancelled);
    m_context.currentStage = isCancel ? PipelineStage::Cancelled : PipelineStage::Error;

    if (isCancel) {
        updateProgress(PipelineStage::Cancelled);
        reportProgress();
        return error;
    }

    if (!m_config.enableRecovery) {
        updateProgress(PipelineStage::Error);
        reportProgress();
        return error;
    }

    int retries = m_stageRetryCounts[stage]++;
    RecoveryContext rc(stage, error, std::string(toString(error)));
    rc.retryCount = retries;

    auto result = performRecovery(rc);
    if (result.isError()) {
        updateProgress(PipelineStage::Error);
        reportProgress();
        return result;
    }

    return {};
}

Result<void> BootPipeline::performRecovery(const RecoveryContext& rc) {
    if (!m_recoveryStrategy) {
        return rc.error;
    }

    if (m_recoveryAttempts >= kMaxRecoveryAttempts) {
        if (m_context.logger) {
            m_context.logger->warn("BootPipeline", "max recovery attempts reached");
        }
        return rc.error;
    }

    auto action = m_recoveryStrategy->decide(rc);

    switch (action) {
        case RecoveryAction::Retry: {
            ++m_recoveryAttempts;
            if (m_context.logger) {
                m_context.logger->warn("BootPipeline",
                    std::string("retrying stage: ") + std::string(toString(rc.failedStage)));
            }
            m_context.currentStage = rc.failedStage;
            return {};
        }
        case RecoveryAction::RestartStage: {
            ++m_recoveryAttempts;
            m_stageRetryCounts[rc.failedStage] = 0;
            if (m_context.logger) {
                m_context.logger->warn("BootPipeline",
                    std::string("restarting stage: ") + std::string(toString(rc.failedStage)));
            }
            m_context.currentStage = rc.failedStage;
            return {};
        }
        case RecoveryAction::Rollback: {
            ++m_recoveryAttempts;
            auto target = m_recoveryStrategy->rollbackTarget(rc.failedStage, 2);
            if (m_context.logger) {
                m_context.logger->warn("BootPipeline",
                    std::string("rolling back from ") + std::string(toString(rc.failedStage))
                    + std::string(" to ") + std::string(toString(target)));
            }
            m_context.currentStage = target;
            return {};
        }
        case RecoveryAction::Abort:
        default: {
            if (m_context.logger) {
                m_context.logger->error("BootPipeline",
                    std::string("aborting at stage: ") + std::string(toString(rc.failedStage)));
            }
            m_context.currentStage = PipelineStage::Error;
            return rc.error;
        }
    }
}

void BootPipeline::updateProgress(PipelineStage stage) {
    auto& progress = m_context.progress;
    progress.stage = std::string(toString(stage));

    if (stage == PipelineStage::Ready) {
        progress.percentage = 100.0;
        progress.currentOperation = "Boot completed";
    } else if (stage == PipelineStage::Error) {
        progress.currentOperation = "Boot failed";
        progress.isIndeterminate = false;
    } else if (stage == PipelineStage::Cancelled) {
        progress.currentOperation = "Boot cancelled";
    } else {
        int idx = stageIndex(stage);
        if (idx >= 0) {
            progress.percentage = (static_cast<double>(idx) / kStageOrder.size()) * 100.0;
        }
        progress.currentOperation = std::string(toString(stage));
    }

    progress.cancelable = (stage != PipelineStage::Ready && stage != PipelineStage::Error);
}

void BootPipeline::reportProgress() {
    if (m_progressCb) {
        m_progressCb(m_context);
    }
}

int BootPipeline::stageIndex(PipelineStage stage) {
    for (std::size_t i = 0; i < kStageOrder.size(); ++i) {
        if (kStageOrder[i] == stage) return static_cast<int>(i);
    }
    return -1;
}

PipelineStage BootPipeline::nextStage(PipelineStage current) {
    for (std::size_t i = 0; i < kStageOrder.size(); ++i) {
        if (kStageOrder[i] == current) {
            if (i + 1 < kStageOrder.size()) {
                return kStageOrder[i + 1];
            }
            return PipelineStage::Ready;
        }
    }
    if (current == PipelineStage::Disconnected) {
        return PipelineStage::Connected;
    }
    return PipelineStage::Error;
}

PipelineStage BootPipeline::previousStage(PipelineStage current, int levels) {
    int idx = stageIndex(current);
    if (idx < 0) {
        if (current == PipelineStage::Error || current == PipelineStage::Cancelled) {
            return PipelineStage::Disconnected;
        }
        return PipelineStage::Disconnected;
    }
    int target = std::max(0, idx - levels);
    return kStageOrder[static_cast<std::size_t>(target)];
}

} // namespace pipeline
} // namespace mbootcore
