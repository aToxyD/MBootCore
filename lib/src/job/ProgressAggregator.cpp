#include <mbootcore/job/ProgressAggregator.hpp>

#include <algorithm>

namespace mbootcore {
namespace job {

ProgressAggregator::ProgressAggregator() = default;

void ProgressAggregator::setCallback(AggregatedProgressCallback callback) {
    m_callback = std::move(callback);
}

void ProgressAggregator::registerJob(const std::string& jobId, JobType type,
                                      uint64_t totalBytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    JobProgress jp;
    jp.type = type;
    jp.totalBytes = totalBytes;
    m_jobs[jobId] = jp;
}

void ProgressAggregator::unregisterJob(const std::string& jobId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.erase(jobId);
}

void ProgressAggregator::updateJobProgress(const std::string& jobId,
                                            const ProgressInfo& info) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it == m_jobs.end()) return;
        it->second.totalBytes = info.totalBytes;
        it->second.transferredBytes = info.transferredBytes;
        it->second.percentage = info.percentage;
        it->second.operation = info.currentOperation;
        it->second.isIndeterminate = info.isIndeterminate;
    }
    report();
}

void ProgressAggregator::markJobCompleted(const std::string& jobId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it == m_jobs.end()) return;
        it->second.completed = true;
        it->second.percentage = 100.0;
        m_totalCompleted++;
    }
    report();
}

void ProgressAggregator::markJobFailed(const std::string& jobId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it == m_jobs.end()) return;
        it->second.failed = true;
        m_totalFailed++;
    }
    report();
}

AggregatedProgress ProgressAggregator::current() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    AggregatedProgress ap;
    ap.totalJobs = m_totalJobs;
    ap.completedJobs = m_totalCompleted;
    ap.failedJobs = m_totalFailed;

    for (const auto& [id, jp] : m_jobs) {
        if (!jp.completed && !jp.failed) {
            ap.activeJobs++;
            ap.currentJobId = id;
            ap.currentJobType = jp.type;
            ap.currentOperation = jp.operation;
            ap.isIndeterminate = jp.isIndeterminate;
        }
        if (jp.totalBytes > 0 && !jp.isIndeterminate) {
            ap.totalBytes += jp.totalBytes;
            ap.transferredBytes += jp.transferredBytes;
        }
    }

    if (ap.totalBytes > 0) {
        ap.overallPercentage = 100.0 * static_cast<double>(ap.transferredBytes) /
                               static_cast<double>(ap.totalBytes);
    } else if (m_totalJobs > 0) {
        ap.overallPercentage = 100.0 * static_cast<double>(m_totalCompleted + m_totalFailed) /
                               static_cast<double>(m_totalJobs);
    }

    return ap;
}

void ProgressAggregator::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.clear();
    m_totalJobs = 0;
    m_totalCompleted = 0;
    m_totalFailed = 0;
    m_totalBytes = 0;
}

void ProgressAggregator::report() {
    if (m_callback) {
        m_callback(current());
    }
}

} // namespace job
} // namespace mbootcore
