#include <mbootcore/workflow/WorkflowHistory.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

namespace mbootcore {
namespace workflow {

void WorkflowHistory::addEntry(const WorkflowHistoryEntry& entry) {
    m_entries.push_back(entry);
    if (m_entries.size() > kMaxEntries) {
        m_entries.erase(m_entries.begin(),
                       m_entries.begin() + static_cast<ptrdiff_t>(m_entries.size() - kMaxEntries));
    }
}

void WorkflowHistory::clear() {
    m_entries.clear();
}

std::vector<WorkflowHistoryEntry> WorkflowHistory::recent(size_t count) const {
    if (count >= m_entries.size()) return m_entries;
    return std::vector<WorkflowHistoryEntry>(
        m_entries.end() - static_cast<ptrdiff_t>(count), m_entries.end());
}

std::vector<WorkflowHistoryEntry> WorkflowHistory::filter(bool success) const {
    std::vector<WorkflowHistoryEntry> result;
    for (const auto& e : m_entries) {
        if (e.success == success) {
            result.push_back(e);
        }
    }
    return result;
}

std::vector<WorkflowHistoryEntry> WorkflowHistory::filterByVendor(
    discovery::Vendor vendor) const {
    std::vector<WorkflowHistoryEntry> result;
    for (const auto& e : m_entries) {
        if (e.vendor == vendor) {
            result.push_back(e);
        }
    }
    return result;
}

std::vector<WorkflowHistoryEntry> WorkflowHistory::filterByProtocol(
    discovery::ProtocolType protocol) const {
    std::vector<WorkflowHistoryEntry> result;
    for (const auto& e : m_entries) {
        if (e.protocol == protocol) {
            result.push_back(e);
        }
    }
    return result;
}

size_t WorkflowHistory::successCount() const {
    size_t count = 0;
    for (const auto& e : m_entries) {
        if (e.success) ++count;
    }
    return count;
}

size_t WorkflowHistory::failureCount() const {
    size_t count = 0;
    for (const auto& e : m_entries) {
        if (!e.success) ++count;
    }
    return count;
}

std::string WorkflowHistory::exportText() const {
    std::ostringstream oss;
    oss << "Workflow History (" << m_entries.size() << " entries)\n";
    oss << std::string(80, '=') << "\n";
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        oss << "[" << (i + 1) << "] "
            << (e.success ? "OK" : "FAIL")
            << " | " << e.workflowId
            << " | " << e.duration.count() << "ms"
            << " | Errors: " << e.errors.size()
            << " | Warnings: " << e.warnings.size()
            << "\n";
    }
    return oss.str();
}

std::string WorkflowHistory::exportJson() const {
    try {
        nlohmann::json j;
        auto entries = nlohmann::json::array();
        for (const auto& e : m_entries) {
            nlohmann::json entry;
            entry["id"] = e.workflowId;
            entry["success"] = e.success;
            entry["duration_ms"] = e.duration.count();
            entry["errors"] = e.errors.size();
            entries.push_back(std::move(entry));
        }
        j["entries"] = std::move(entries);
        return j.dump(2);
    } catch (...) {
        return "{}";
    }
}

} // namespace workflow
} // namespace mbootcore
