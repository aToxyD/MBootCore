#pragma once

#include <mbootcore/generic/ProgressInfo.hpp>
#include <mbootcore/job/JobTypes.hpp>

#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <map>

namespace mbootcore {
namespace job {

struct AggregatedProgress {
    std::string currentJobId;
    JobType currentJobType{JobType::Flash};
    int totalJobs{0};
    int completedJobs{0};
    int failedJobs{0};
    int activeJobs{0};

    uint64_t totalBytes{0};
    uint64_t transferredBytes{0};
    double overallPercentage{0.0};
    double speedBps{0.0};
    double estimatedSeconds{0.0};

    std::string currentOperation;
    bool isIndeterminate{false};
    bool cancelable{true};
};

using AggregatedProgressCallback = std::function<void(const AggregatedProgress& progress)>;

class ProgressAggregator {
public:
    ProgressAggregator();

    void setCallback(AggregatedProgressCallback callback);

    void registerJob(const std::string& jobId, JobType type, uint64_t totalBytes = 0);
    void unregisterJob(const std::string& jobId);

    void updateJobProgress(const std::string& jobId, const ProgressInfo& info);
    void markJobCompleted(const std::string& jobId);
    void markJobFailed(const std::string& jobId);

    void setTotalJobs(int total) { m_totalJobs = total; }
    void setTotalBytes(uint64_t bytes) { m_totalBytes = bytes; }

    AggregatedProgress current() const;

    void reset();

private:
    struct JobProgress {
        JobType type;
        uint64_t totalBytes{0};
        uint64_t transferredBytes{0};
        double percentage{0.0};
        bool completed{false};
        bool failed{false};
        std::string operation;
        bool isIndeterminate{false};
    };

    void report();

    AggregatedProgressCallback m_callback;
    std::map<std::string, JobProgress> m_jobs;
    int m_totalJobs{0};
    int m_totalCompleted{0};
    int m_totalFailed{0};
    uint64_t m_totalBytes{0};
    mutable std::mutex m_mutex;
};

} // namespace job
} // namespace mbootcore
