#pragma once

#include <mbootcore/job/JobTypes.hpp>

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <mutex>

namespace mbootcore {
namespace job {

struct JobHistoryEntry {
    std::string jobId;
    JobType type;
    JobConfig config;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::chrono::milliseconds duration{0};
    bool success{false};
    ErrorCode error{ErrorCode::Success};
    std::string errorMessage;
    JobStatistics statistics;
    std::vector<std::string> logEntries;
};

struct HistoryFilter {
    std::vector<JobType> types;
    std::vector<bool> successFilter;
    std::chrono::steady_clock::time_point fromTime;
    std::chrono::steady_clock::time_point toTime;
    std::string jobIdFilter;
    int maxResults{0};
};

class JobHistory {
public:
    JobHistory() = default;

    void addEntry(const JobHistoryEntry& entry);
    void clear();

    std::vector<JobHistoryEntry> all() const;
    std::vector<JobHistoryEntry> recent(int count = 10) const;
    std::vector<JobHistoryEntry> find(const HistoryFilter& filter) const;
    std::vector<JobHistoryEntry> completed() const;
    std::vector<JobHistoryEntry> failed() const;

    std::size_t totalCount() const noexcept { return m_entries.size(); }
    std::size_t successCount() const;
    std::size_t failureCount() const;

    void setMaxEntries(std::size_t max) { m_maxEntries = max; }
    std::size_t maxEntries() const noexcept { return m_maxEntries; }

private:
    std::vector<JobHistoryEntry> m_entries;
    std::size_t m_maxEntries{1000};
    mutable std::mutex m_mutex;
};

} // namespace job
} // namespace mbootcore
