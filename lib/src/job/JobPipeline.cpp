#include <mbootcore/job/JobPipeline.hpp>

#include <algorithm>
#include <thread>

namespace mbootcore {
namespace job {

JobPipeline::JobPipeline() = default;

JobPipeline::~JobPipeline() {
    cancel();
}

void JobPipeline::addJob(std::unique_ptr<IJob> job) {
    JobEntry entry;
    entry.job = std::move(job);
    m_jobs.push_back(std::move(entry));
}

void JobPipeline::insertJob(std::size_t index, std::unique_ptr<IJob> job) {
    if (index > m_jobs.size()) index = m_jobs.size();
    JobEntry entry;
    entry.job = std::move(job);
    m_jobs.insert(m_jobs.begin() + static_cast<ptrdiff_t>(index), std::move(entry));
}

void JobPipeline::removeJob(const std::string& jobId) {
    m_jobs.erase(
        std::remove_if(m_jobs.begin(), m_jobs.end(),
                       [&](const JobEntry& entry) {
                           return entry.job->id() == jobId;
                       }),
        m_jobs.end());
}

void JobPipeline::clearJobs() {
    m_jobs.clear();
    m_completedCount = 0;
    m_failedCount = 0;
    m_currentIndex = 0;
}

Result<void> JobPipeline::run(JobContext& context) {
    if (m_running) {
        return ErrorCode::JobAlreadyRunning;
    }

    m_running = true;
    m_paused = false;
    m_cancelled = false;
    m_completedCount = 0;
    m_failedCount = 0;

    m_aggregator.setTotalJobs(static_cast<int>(m_jobs.size()));

    for (std::size_t i = 0; i < m_jobs.size(); i++) {
        if (m_cancelled) break;
        if (m_paused) {
            while (m_paused && !m_cancelled) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (m_cancelled) break;
        }

        m_currentIndex = i;
        auto& entry = m_jobs[i];
        auto* job = entry.job.get();

        m_aggregator.registerJob(job->id(), job->type());
        job->setConfig(entry.job->config());

        auto result = executeJob(i, context);

        if (result.isError()) {
            if (result.error() == ErrorCode::Cancelled) {
                entry.m_failed = true;
                m_failedCount++;
                m_aggregator.markJobFailed(job->id());
                m_running = false;
                return ErrorCode::Cancelled;
            }

            auto* recovery = job->recoveryPolicy();
            if (recovery && recovery->canRollback() && job->canRollback()) {
                (void)rollbackAll(context);
            }
            entry.m_failed = true;
            m_failedCount++;
            m_aggregator.markJobFailed(job->id());

            if (m_jobCb) {
                m_jobCb(job->id(), JobState::Failed, false);
            }

            JobHistoryEntry histEntry;
            histEntry.jobId = job->id();
            histEntry.type = job->type();
            histEntry.config = job->config();
            histEntry.success = false;
            histEntry.error = result.error();
            histEntry.statistics = job->result().statistics;
            m_history.addEntry(histEntry);

            m_running = false;
            return result;
        }

        m_completedCount++;
        m_aggregator.markJobCompleted(job->id());

        if (m_jobCb) {
            m_jobCb(job->id(), JobState::Completed, true);
        }

        JobHistoryEntry histEntry;
        histEntry.jobId = job->id();
        histEntry.type = job->type();
        histEntry.config = job->config();
        histEntry.success = true;
        histEntry.statistics = job->result().statistics;
        m_history.addEntry(histEntry);
    }

    m_running = false;
    return {};
}

void JobPipeline::cancel() {
    m_cancelled = true;
    for (auto& entry : m_jobs) {
        if (entry.job) {
            (void)entry.job->cancel();
        }
    }
}

void JobPipeline::pause() {
    m_paused = true;
}

void JobPipeline::resume() {
    m_paused = false;
}

IJob* JobPipeline::findJob(const std::string& jobId) {
    for (auto& entry : m_jobs) {
        if (entry.job->id() == jobId) return entry.job.get();
    }
    return nullptr;
}

const IJob* JobPipeline::findJob(const std::string& jobId) const {
    for (const auto& entry : m_jobs) {
        if (entry.job->id() == jobId) return entry.job.get();
    }
    return nullptr;
}

std::vector<IJob*> JobPipeline::jobs() {
    std::vector<IJob*> result;
    result.reserve(m_jobs.size());
    for (auto& entry : m_jobs) {
        result.push_back(entry.job.get());
    }
    return result;
}

std::vector<const IJob*> JobPipeline::jobs() const {
    std::vector<const IJob*> result;
    result.reserve(m_jobs.size());
    for (const auto& entry : m_jobs) {
        result.push_back(entry.job.get());
    }
    return result;
}

void JobPipeline::setProgressCallback(PipelineProgressCallback callback) {
    m_progressCb = std::move(callback);
    if (m_progressCb) {
        m_aggregator.setCallback([this](const AggregatedProgress& ap) {
            if (m_progressCb) m_progressCb(ap);
        });
    }
}

void JobPipeline::setJobCallback(PipelineJobCallback callback) {
    m_jobCb = std::move(callback);
}

Result<void> JobPipeline::executeJob(std::size_t index, JobContext& context) {
    auto& entry = m_jobs[index];
    auto* job = entry.job.get();

    context.progressCallback = [this, job](const std::string& id, const ProgressInfo& info) {
        reportProgress(id, info);
    };

    MBOOT_TRY(job->prepare(context));
    MBOOT_TRY(job->execute(context));

    return {};
}

Result<void> JobPipeline::rollbackAll(JobContext& context) {
    ErrorCode lastErr = ErrorCode::Success;
    for (std::size_t i = m_currentIndex; i > 0; i--) {
        auto& entry = m_jobs[i - 1];
        if (entry.job && entry.job->canRollback()) {
            auto rbResult = entry.job->rollback(context);
            if (!rbResult) {
                lastErr = rbResult.error();
            }
        }
    }
    if (lastErr != ErrorCode::Success) {
        return lastErr;
    }
    return {};
}

void JobPipeline::reportProgress(const std::string& jobId, const ProgressInfo& info) {
    m_aggregator.updateJobProgress(jobId, info);
}

} // namespace job
} // namespace mbootcore
