#include <mbootcore/job/JobHistory.hpp>

#include <algorithm>
#include <iterator>

namespace mbootcore {
namespace job {

void JobHistory::addEntry(const JobHistoryEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.push_back(entry);
    if (m_entries.size() > m_maxEntries) {
        m_entries.erase(m_entries.begin(),
                       m_entries.begin() + static_cast<ptrdiff_t>(m_entries.size() - m_maxEntries));
    }
}

void JobHistory::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

std::vector<JobHistoryEntry> JobHistory::all() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries;
}

std::vector<JobHistoryEntry> JobHistory::recent(int count) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_entries.size() <= static_cast<size_t>(count)) {
        return m_entries;
    }
    return std::vector<JobHistoryEntry>(
        m_entries.end() - static_cast<ptrdiff_t>(count), m_entries.end());
}

std::vector<JobHistoryEntry> JobHistory::find(const HistoryFilter& filter) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JobHistoryEntry> result;
    for (const auto& entry : m_entries) {
        if (!filter.types.empty()) {
            auto it = std::find(filter.types.begin(), filter.types.end(), entry.type);
            if (it == filter.types.end()) continue;
        }
        if (!filter.successFilter.empty()) {
            bool match = false;
            for (auto s : filter.successFilter) {
                if (entry.success == s) { match = true; break; }
            }
            if (!match) continue;
        }
        if (filter.fromTime > std::chrono::steady_clock::time_point{} &&
            entry.startTime < filter.fromTime) continue;
        if (filter.toTime > std::chrono::steady_clock::time_point{} &&
            entry.startTime > filter.toTime) continue;
        if (!filter.jobIdFilter.empty() && entry.jobId != filter.jobIdFilter) continue;
        result.push_back(entry);
        if (filter.maxResults > 0 && static_cast<int>(result.size()) >= filter.maxResults) break;
    }
    return result;
}

std::vector<JobHistoryEntry> JobHistory::completed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JobHistoryEntry> result;
    std::copy_if(m_entries.begin(), m_entries.end(), std::back_inserter(result),
                 [](const auto& e) { return e.success; });
    return result;
}

std::vector<JobHistoryEntry> JobHistory::failed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JobHistoryEntry> result;
    std::copy_if(m_entries.begin(), m_entries.end(), std::back_inserter(result),
                 [](const auto& e) { return !e.success; });
    return result;
}

std::size_t JobHistory::successCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<std::size_t>(
        std::count_if(m_entries.begin(), m_entries.end(),
                      [](const auto& e) { return e.success; }));
}

std::size_t JobHistory::failureCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<std::size_t>(
        std::count_if(m_entries.begin(), m_entries.end(),
                      [](const auto& e) { return !e.success; }));
}

} // namespace job
} // namespace mbootcore
