#pragma once

#include <mbootcore/pipeline/PipelineStage.hpp>
#include <mbootcore/pipeline/BootContext.hpp>
#include <mbootcore/pipeline/BootPipelineConfig.hpp>
#include <mbootcore/pipeline/RecoveryStrategy.hpp>

#include <functional>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <vector>
#include <chrono>
#include <string>

namespace mbootcore {
namespace pipeline {

using StageHandler = std::function<Result<void>(BootContext& context)>;

class BootPipeline {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    BootPipeline();
    ~BootPipeline() = default;

    BootPipeline(const BootPipeline&) = delete;
    BootPipeline& operator=(const BootPipeline&) = delete;
    BootPipeline(BootPipeline&&) = delete;
    BootPipeline& operator=(BootPipeline&&) = delete;

    void setConfig(const BootPipelineConfig& config) { m_config = config; }
    const BootPipelineConfig& config() const { return m_config; }

    void setTransport(ITransport* transport) { m_context.transport = transport; }
    void setLogger(ILogger* logger) { m_context.logger = logger; }

    void setHandler(PipelineStage stage, StageHandler handler) {
        m_handlers[stage] = std::move(handler);
    }
    void clearHandler(PipelineStage stage) { m_handlers.erase(stage); }
    bool hasHandler(PipelineStage stage) const { return m_handlers.count(stage) > 0; }

    void setRecoveryStrategy(std::shared_ptr<RecoveryStrategy> strategy) {
        m_recoveryStrategy = std::move(strategy);
    }
    RecoveryStrategy* recoveryStrategy() const { return m_recoveryStrategy.get(); }

    using ProgressCallback = std::function<void(const BootContext& context)>;
    void setProgressCallback(ProgressCallback cb) { m_progressCb = std::move(cb); }

    Result<void> run();
    void cancel();

    PipelineStage currentStage() const { return m_context.currentStage; }
    PipelineStage previousStage() const { return m_context.previousStage; }
    const BootContext& context() const { return m_context; }
    BootContext& context() { return m_context; }

    [[nodiscard]] bool isRunning() const { return m_running; }
    [[nodiscard]] bool isCancelled() const { return m_cancelled; }
    [[nodiscard]] bool isInTerminalState() const {
        return m_context.currentStage == PipelineStage::Ready
            || m_context.currentStage == PipelineStage::Error
            || m_context.currentStage == PipelineStage::Cancelled;
    }

    Result<void> retry();
    Result<void> reset();
    ErrorCode lastError() const { return m_context.lastError; }

    static constexpr std::size_t stageCount() {
        return kStageOrder.size();
    }

    static constexpr PipelineStage stageAtIndex(std::size_t index) {
        return index < kStageOrder.size() ? kStageOrder[index] : PipelineStage::Error;
    }

private:
    static constexpr std::array<PipelineStage, 12> kStageOrder = {{
        PipelineStage::Connected,
        PipelineStage::SaharaHandshake,
        PipelineStage::VersionNegotiation,
        PipelineStage::DeviceDiscovery,
        PipelineStage::LoaderSelection,
        PipelineStage::ElfParsing,
        PipelineStage::MemoryImageBuild,
        PipelineStage::ProgrammerUpload,
        PipelineStage::ProgrammerExecute,
        PipelineStage::FirehoseDetection,
        PipelineStage::FirehoseConfiguration,
        PipelineStage::Ready
    }};

    BootContext m_context;
    BootPipelineConfig m_config;
    std::unordered_map<PipelineStage, StageHandler> m_handlers;
    std::shared_ptr<RecoveryStrategy> m_recoveryStrategy;

    std::atomic<bool> m_cancelled{false};
    std::atomic<bool> m_running{false};

    ProgressCallback m_progressCb;

    std::unordered_map<PipelineStage, int> m_stageRetryCounts;
    PipelineStage m_recoveryTarget{PipelineStage::Disconnected};
    int m_recoveryAttempts{0};

    static constexpr int kMaxRecoveryAttempts{5};

    Result<void> executeStage(PipelineStage stage);
    Result<void> handleStageSuccess(PipelineStage stage);
    Result<void> handleStageFailure(PipelineStage stage, const ErrorCode& error);
    Result<void> performRecovery(const RecoveryContext& rc);

    void updateProgress(PipelineStage stage);
    void reportProgress();

    static int stageIndex(PipelineStage stage);
    static PipelineStage nextStage(PipelineStage current);
    static PipelineStage previousStage(PipelineStage current, int levels = 1);
};

} // namespace pipeline
} // namespace mbootcore
