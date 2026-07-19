#pragma once

#include <mbootcore/job/IJob.hpp>
#include <mbootcore/job/ProgressAggregator.hpp>
#include <mbootcore/job/JobHistory.hpp>

#include <memory>
#include <vector>
#include <queue>
#include <string>
#include <functional>
#include <atomic>
#include <unordered_map>

namespace mbootcore {
namespace job {

using PipelineProgressCallback = std::function<void(const AggregatedProgress& progress)>;
using PipelineJobCallback = std::function<void(const std::string& jobId, JobState state, bool success)>;

class JobPipeline {
public:
    JobPipeline();
    ~JobPipeline();

    JobPipeline(const JobPipeline&) = delete;
    JobPipeline& operator=(const JobPipeline&) = delete;
    JobPipeline(JobPipeline&&) = delete;
    JobPipeline& operator=(JobPipeline&&) = delete;

    void addJob(std::unique_ptr<IJob> job);
    void insertJob(std::size_t index, std::unique_ptr<IJob> job);
    void removeJob(const std::string& jobId);
    void clearJobs();

    Result<void> run(JobContext& context);
    void cancel();
    void pause();
    void resume();

    bool isRunning() const noexcept { return m_running; }
    bool isPaused() const noexcept { return m_paused; }
    bool isCancelled() const noexcept { return m_cancelled; }

    std::size_t jobCount() const noexcept { return m_jobs.size(); }
    std::size_t completedCount() const noexcept { return m_completedCount; }
    std::size_t failedCount() const noexcept { return m_failedCount; }

    IJob* findJob(const std::string& jobId);
    const IJob* findJob(const std::string& jobId) const;
    std::vector<IJob*> jobs();
    std::vector<const IJob*> jobs() const;

    void setProgressCallback(PipelineProgressCallback callback);
    void setJobCallback(PipelineJobCallback callback);

    const JobHistory& history() const noexcept { return m_history; }
    JobHistory& history() noexcept { return m_history; }

private:
    Result<void> executeJob(std::size_t index, JobContext& context);
    Result<void> rollbackAll(JobContext& context);

    struct JobEntry {
        std::unique_ptr<IJob> job;
        bool completed{false};
        bool m_failed{false};
    };

    std::vector<JobEntry> m_jobs;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_cancelled{false};

    std::size_t m_currentIndex{0};
    std::size_t m_completedCount{0};
    std::size_t m_failedCount{0};

    PipelineProgressCallback m_progressCb;
    PipelineJobCallback m_jobCb;
    ProgressAggregator m_aggregator;
    JobHistory m_history;

    void reportProgress(const std::string& jobId, const ProgressInfo& info);
};

} // namespace job
} // namespace mbootcore
