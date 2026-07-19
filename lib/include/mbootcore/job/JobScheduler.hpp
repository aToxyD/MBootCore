#pragma once

#include <mbootcore/job/IJob.hpp>
#include <mbootcore/job/JobPipeline.hpp>
#include <mbootcore/job/JobHistory.hpp>

#include <queue>
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <map>

namespace mbootcore {
namespace job {

struct SchedulerStats {
    std::size_t totalScheduled{0};
    std::size_t totalCompleted{0};
    std::size_t totalFailed{0};
    std::size_t totalCancelled{0};
    std::size_t queueSize{0};
    std::size_t activeCount{0};
    bool isRunning{false};
};

enum class SchedulerState : uint32_t {
    Idle,
    Running,
    Paused,
    Shutdown
};

class JobScheduler {
public:
    explicit JobScheduler(JobContext& context);
    ~JobScheduler();

    JobScheduler(const JobScheduler&) = delete;
    JobScheduler& operator=(const JobScheduler&) = delete;

    void enqueue(std::unique_ptr<IJob> job);
    void enqueueWithDependencies(std::unique_ptr<IJob> job,
                                  const std::vector<std::string>& dependencies);

    void start();
    void stop();
    void pause();
    void resume();
    void cancel(const std::string& jobId);

    SchedulerState state() const noexcept { return m_state; }
    SchedulerStats stats() const;

    void setPipelineCallback(PipelineProgressCallback callback);
    void setPipelineJobCallback(PipelineJobCallback callback);

    const JobHistory& history() const noexcept { return m_pipeline.history(); }
    JobHistory& history() noexcept { return m_pipeline.history(); }

    JobPipeline& pipeline() noexcept { return m_pipeline; }
    const JobPipeline& pipeline() const noexcept { return m_pipeline; }

private:
    struct QueuedJob {
        std::unique_ptr<IJob> job;
        std::vector<std::string> dependencies;
        std::chrono::steady_clock::time_point enqueuedAt;
    };

    void processQueue();

    JobContext& m_context;
    JobPipeline m_pipeline;
    std::queue<QueuedJob> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<SchedulerState> m_state{SchedulerState::Idle};
    std::unique_ptr<std::thread> m_worker;

    std::atomic<std::size_t> m_totalScheduled{0};
    std::atomic<std::size_t> m_totalCompleted{0};
    std::atomic<std::size_t> m_totalFailed{0};
    std::atomic<std::size_t> m_totalCancelled{0};
    std::size_t m_activeCount{0};

    void updateCounters();
};

} // namespace job
} // namespace mbootcore
