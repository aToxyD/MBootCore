#include <mbootcore/job/JobScheduler.hpp>

#include <algorithm>

namespace mbootcore {
namespace job {

JobScheduler::JobScheduler(JobContext& context)
    : m_context(context) {
}

JobScheduler::~JobScheduler() {
    stop();
}

void JobScheduler::enqueue(std::unique_ptr<IJob> job) {
    std::lock_guard<std::mutex> lock(m_mutex);
    QueuedJob qj;
    qj.job = std::move(job);
    qj.enqueuedAt = std::chrono::steady_clock::now();
    m_queue.push(std::move(qj));
    m_totalScheduled++;
}

void JobScheduler::enqueueWithDependencies(std::unique_ptr<IJob> job,
                                            const std::vector<std::string>& dependencies) {
    std::lock_guard<std::mutex> lock(m_mutex);
    QueuedJob qj;
    qj.job = std::move(job);
    qj.dependencies = dependencies;
    qj.enqueuedAt = std::chrono::steady_clock::now();
    m_queue.push(std::move(qj));
    m_totalScheduled++;
}

void JobScheduler::start() {
    SchedulerState expected = SchedulerState::Idle;
    if (!m_state.compare_exchange_strong(expected, SchedulerState::Running)) {
        return;
    }
    m_worker = std::make_unique<std::thread>(&JobScheduler::processQueue, this);
}

void JobScheduler::stop() {
    m_state = SchedulerState::Shutdown;
    m_pipeline.cancel();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<QueuedJob> empty;
        std::swap(m_queue, empty);
    }
    m_cv.notify_one();
    if (m_worker && m_worker->joinable()) {
        m_worker->join();
    }
}

void JobScheduler::pause() {
    m_state = SchedulerState::Paused;
    m_pipeline.pause();
}

void JobScheduler::resume() {
    if (m_state == SchedulerState::Paused) {
        m_state = SchedulerState::Running;
        m_pipeline.resume();
        m_cv.notify_one();
    }
}

void JobScheduler::cancel(const std::string& jobId) {
    (void)jobId;
    m_pipeline.cancel();
}

SchedulerStats JobScheduler::stats() const {
    SchedulerStats s;
    s.totalScheduled = m_totalScheduled.load();
    s.totalCompleted = m_totalCompleted.load();
    s.totalFailed = m_totalFailed.load();
    s.totalCancelled = m_totalCancelled.load();
    s.isRunning = m_state == SchedulerState::Running;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        s.queueSize = m_queue.size();
    }
    s.activeCount = m_pipeline.isRunning() ? 1 : 0;
    return s;
}

void JobScheduler::setPipelineCallback(PipelineProgressCallback callback) {
    m_pipeline.setProgressCallback(std::move(callback));
}

void JobScheduler::setPipelineJobCallback(PipelineJobCallback callback) {
    m_pipeline.setJobCallback(std::move(callback));
}

void JobScheduler::processQueue() {
    while (m_state == SchedulerState::Running) {
        QueuedJob qj;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                m_cv.wait_for(lock, std::chrono::milliseconds(100),
                    [this]() { return !m_queue.empty() || m_state != SchedulerState::Running; });
                if (m_queue.empty()) continue;
            }
            if (m_state != SchedulerState::Running) break;
            qj = std::move(m_queue.front());
            m_queue.pop();
        }

        if (!qj.job) continue;

        if (m_state == SchedulerState::Paused) {
            while (m_state == SchedulerState::Paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (m_state != SchedulerState::Running) break;
        }

        m_pipeline.clearJobs();
        m_pipeline.addJob(std::move(qj.job));

        auto result = m_pipeline.run(m_context);
        if (result.isOk()) {
            m_totalCompleted++;
        } else if (result.error() == ErrorCode::Cancelled) {
            m_totalCancelled++;
        } else {
            m_totalFailed++;
        }

        updateCounters();
    }

    m_state = SchedulerState::Idle;
}

void JobScheduler::updateCounters() {
    m_totalCompleted = m_pipeline.completedCount();
    m_totalFailed = m_pipeline.failedCount();
}

} // namespace job
} // namespace mbootcore
