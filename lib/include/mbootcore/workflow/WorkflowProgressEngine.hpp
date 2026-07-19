#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <mbootcore/pipeline/PipelineStage.hpp>
#include <mbootcore/job/JobTypes.hpp>

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <unordered_map>

namespace mbootcore {
namespace workflow {

class WorkflowProgressEngine {
public:
    WorkflowProgressEngine() = default;

    void reset();

    void setTotalSteps(size_t count);
    void setCurrentStep(const std::string& name, WorkflowStepType type);

    void updateStepProgress(double progress);
    void updateBytesTransferred(uint64_t bytes);

    void markStepCompleted();
    void markStepFailed(const std::string& error);
    void markCancelled();

    WorkflowProgress currentProgress() const noexcept;
    double overallPercentage() const noexcept;
    std::chrono::seconds estimatedTimeRemaining() const;

    using ProgressCallback = std::function<void(const WorkflowProgress& progress)>;
    void setCallback(ProgressCallback cb) { m_callback = std::move(cb); }

private:
    void recalculate();
    void notify();

    size_t m_totalSteps{0};
    size_t m_completedSteps{0};
    size_t m_failedSteps{0};
    double m_currentStepProgress{0.0};
    uint64_t m_bytesTransferred{0};
    std::string m_currentStepName;
    WorkflowStepType m_currentStepType{WorkflowStepType::Custom};

    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastUpdate;

    ProgressCallback m_callback;
};

} // namespace workflow
} // namespace mbootcore
