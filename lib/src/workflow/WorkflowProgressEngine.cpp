#include <mbootcore/workflow/WorkflowProgressEngine.hpp>

namespace mbootcore {
namespace workflow {

void WorkflowProgressEngine::reset() {
    m_totalSteps = 0;
    m_completedSteps = 0;
    m_failedSteps = 0;
    m_currentStepProgress = 0.0;
    m_bytesTransferred = 0;
    m_currentStepName.clear();
    m_currentStepType = WorkflowStepType::Custom;
    m_startTime = std::chrono::steady_clock::time_point{};
    m_lastUpdate = std::chrono::steady_clock::time_point{};
}

void WorkflowProgressEngine::setTotalSteps(size_t count) {
    m_totalSteps = count;
    m_startTime = std::chrono::steady_clock::now();
    m_lastUpdate = m_startTime;
}

void WorkflowProgressEngine::setCurrentStep(const std::string& name,
                                              WorkflowStepType type) {
    m_currentStepName = name;
    m_currentStepType = type;
    m_currentStepProgress = 0.0;
    m_lastUpdate = std::chrono::steady_clock::now();
    notify();
}

void WorkflowProgressEngine::updateStepProgress(double progress) {
    m_currentStepProgress = progress;
    m_lastUpdate = std::chrono::steady_clock::now();
    notify();
}

void WorkflowProgressEngine::updateBytesTransferred(uint64_t bytes) {
    m_bytesTransferred = bytes;
    m_lastUpdate = std::chrono::steady_clock::now();
    notify();
}

void WorkflowProgressEngine::markStepCompleted() {
    ++m_completedSteps;
    m_currentStepProgress = 100.0;
    m_lastUpdate = std::chrono::steady_clock::now();
    notify();
}

void WorkflowProgressEngine::markStepFailed(const std::string& error) {
    (void)error;
    ++m_failedSteps;
    m_currentStepProgress = 0.0;
    m_lastUpdate = std::chrono::steady_clock::now();
    notify();
}

void WorkflowProgressEngine::markCancelled() {
    m_currentStepName = "Cancelled";
    m_currentStepProgress = 0.0;
    m_lastUpdate = std::chrono::steady_clock::now();
    notify();
}

WorkflowProgress WorkflowProgressEngine::currentProgress() const noexcept {
    WorkflowProgress p;
    p.overallProgress = overallPercentage();
    p.currentStep = m_currentStepName;
    p.currentStepProgress = m_currentStepProgress;
    p.eta = estimatedTimeRemaining();
    p.bytesTransferred = m_bytesTransferred;
    return p;
}

double WorkflowProgressEngine::overallPercentage() const noexcept {
    if (m_totalSteps == 0) return 0.0;
    double completedWeight = static_cast<double>(m_completedSteps) / m_totalSteps * 100.0;
    if (m_completedSteps < m_totalSteps) {
        double stepWeight = (1.0 / m_totalSteps) * m_currentStepProgress;
        return completedWeight + stepWeight;
    }
    return 100.0;
}

std::chrono::seconds WorkflowProgressEngine::estimatedTimeRemaining() const {
    if (m_completedSteps == 0) return std::chrono::seconds(0);
    if (m_completedSteps >= m_totalSteps) return std::chrono::seconds(0);

    auto now = std::chrono::steady_clock::now();
    if (m_startTime == std::chrono::steady_clock::time_point{}) return std::chrono::seconds(0);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_startTime);
    if (elapsed.count() <= 0) return std::chrono::seconds(0);

    double stepsPerMs = static_cast<double>(m_completedSteps) / elapsed.count();
    if (stepsPerMs <= 0) return std::chrono::seconds(0);

    auto remainingMs = static_cast<long long>(
        (m_totalSteps - m_completedSteps) / stepsPerMs);
    return std::chrono::seconds(std::max(0LL, remainingMs / 1000));
}

void WorkflowProgressEngine::recalculate() {
    notify();
}

void WorkflowProgressEngine::notify() {
    if (m_callback) {
        m_callback(currentProgress());
    }
}

} // namespace workflow
} // namespace mbootcore
