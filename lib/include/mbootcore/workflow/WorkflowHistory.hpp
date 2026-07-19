#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/domain/Error.hpp>

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <algorithm>

namespace mbootcore {
namespace workflow {

struct WorkflowHistoryEntry {
    std::string workflowId;
    std::chrono::steady_clock::time_point timestamp;
    WorkflowState finalState{WorkflowState::Created};
    WorkflowPriority priority{WorkflowPriority::Normal};
    discovery::Vendor vendor{discovery::Vendor::Unknown};
    discovery::ProtocolType protocol{discovery::ProtocolType::Unknown};
    std::string firmwarePackage;
    std::string deviceDescriptor;
    std::chrono::milliseconds duration{0};
    bool success{false};
    ErrorCode errorCode{ErrorCode::Success};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    WorkflowStatistics statistics;

    std::string deviceInfo() const {
        return deviceDescriptor.empty() ? "unknown" : deviceDescriptor;
    }
};

class WorkflowHistory {
public:
    WorkflowHistory() = default;

    void addEntry(const WorkflowHistoryEntry& entry);
    void clear();

    std::vector<WorkflowHistoryEntry> recent(size_t count = 10) const;
    std::vector<WorkflowHistoryEntry> filter(bool success) const;
    std::vector<WorkflowHistoryEntry> filterByVendor(discovery::Vendor vendor) const;
    std::vector<WorkflowHistoryEntry> filterByProtocol(discovery::ProtocolType protocol) const;

    size_t totalCount() const noexcept { return m_entries.size(); }
    size_t successCount() const;
    size_t failureCount() const;

    std::string exportText() const;
    std::string exportJson() const;

    bool empty() const noexcept { return m_entries.empty(); }

private:
    std::vector<WorkflowHistoryEntry> m_entries;
    static constexpr size_t kMaxEntries{1000};
};

} // namespace workflow
} // namespace mbootcore
