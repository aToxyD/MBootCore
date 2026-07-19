#include <mbootcore/memory/MemoryTracker.hpp>

#include <unordered_map>
#include <string>
#include <sstream>
#include <chrono>

namespace mbootcore { namespace memory {

struct MemoryTracker::Impl {
    std::unordered_map<void*, AllocationRecord> allocations;
    size_t currentUsage{0};
    size_t peakUsage{0};
    uint64_t totalAllocs{0};
    uint64_t totalDeallocs{0};
    size_t largeBufferThreshold{1024 * 1024};
};

MemoryTracker::MemoryTracker() : m_impl(std::make_unique<Impl>()) {}
MemoryTracker::~MemoryTracker() = default;
MemoryTracker::MemoryTracker(MemoryTracker&&) noexcept = default;
MemoryTracker& MemoryTracker::operator=(MemoryTracker&&) noexcept = default;

void MemoryTracker::trackAllocation(void* address, size_t size, const std::string& label) {
    if (!address) return;

    AllocationRecord rec;
    rec.address = address;
    rec.size = size;
    rec.label = label;
    rec.timestamp = std::chrono::steady_clock::now();
    rec.freed = false;

    m_impl->allocations[address] = std::move(rec);
    m_impl->currentUsage += size;
    m_impl->totalAllocs++;

    if (m_impl->currentUsage > m_impl->peakUsage) {
        m_impl->peakUsage = m_impl->currentUsage;
    }
}

void MemoryTracker::trackDeallocation(void* address) {
    if (!address) return;

    const auto it = m_impl->allocations.find(address);
    if (it == m_impl->allocations.end()) return;

    m_impl->currentUsage -= it->second.size;
    m_impl->totalDeallocs++;
    it->second.freed = true;
    m_impl->allocations.erase(it);
}

Result<MemoryReport> MemoryTracker::report() const {
    MemoryReport r;
    r.currentUsage = m_impl->currentUsage;
    r.peakUsage = m_impl->peakUsage;
    r.totalAllocations = m_impl->totalAllocs;
    r.totalDeallocations = m_impl->totalDeallocs;
    r.largeBufferThreshold = m_impl->largeBufferThreshold;
    r.timestamp = std::chrono::steady_clock::now();

    for (const auto& [addr, rec] : m_impl->allocations) {
        if (!rec.freed) {
            r.liveObjects.push_back(rec);
            if (rec.size >= m_impl->largeBufferThreshold) {
                r.largeBuffers.push_back(rec);
            }
        }
    }

    r.liveAllocations = r.liveObjects.size();
    return r;
}

Result<MemoryReport> MemoryTracker::snapshot() const {
    return report();
}

Result<std::string> MemoryTracker::exportReport() const {
    const auto rep = report();
    if (rep.isError()) {
        return rep.error();
    }

    const auto& r = rep.value();
    std::ostringstream ss;
    ss << "Memory Report\n"
       << "=============\n"
       << "Current Usage: " << r.currentUsage << " bytes\n"
       << "Peak Usage: " << r.peakUsage << " bytes\n"
       << "Total Allocations: " << r.totalAllocations << "\n"
       << "Total Deallocations: " << r.totalDeallocations << "\n"
       << "Live Allocations: " << r.liveAllocations << "\n"
       << "Large Buffer Threshold: " << r.largeBufferThreshold << " bytes\n"
       << "\nLive Objects:\n";

    for (const auto& obj : r.liveObjects) {
        ss << "  [" << obj.label << "] addr=" << obj.address
           << " size=" << obj.size << " bytes\n";
    }

    ss << "\nLarge Buffers:\n";
    for (const auto& buf : r.largeBuffers) {
        ss << "  [" << buf.label << "] addr=" << buf.address
           << " size=" << buf.size << " bytes\n";
    }

    return ss.str();
}

size_t MemoryTracker::currentUsage() const {
    return m_impl->currentUsage;
}

size_t MemoryTracker::peakUsage() const {
    return m_impl->peakUsage;
}

uint64_t MemoryTracker::totalAllocations() const {
    return m_impl->totalAllocs;
}

uint64_t MemoryTracker::totalDeallocations() const {
    return m_impl->totalDeallocs;
}

size_t MemoryTracker::liveAllocationCount() const {
    size_t count = 0;
    for (const auto& [addr, rec] : m_impl->allocations) {
        if (!rec.freed) ++count;
    }
    return count;
}

void MemoryTracker::setLargeBufferThreshold(size_t bytes) {
    m_impl->largeBufferThreshold = bytes;
}

void MemoryTracker::reset() {
    m_impl->allocations.clear();
    m_impl->currentUsage = 0;
    m_impl->peakUsage = 0;
    m_impl->totalAllocs = 0;
    m_impl->totalDeallocs = 0;
}

ScopedMemoryTrack::ScopedMemoryTrack(MemoryTracker& tracker, size_t size, const std::string& label)
    : m_tracker(tracker), m_size(size) {
    m_address = std::malloc(size);
    if (m_address) {
        m_tracker.trackAllocation(m_address, m_size, label);
    }
}

ScopedMemoryTrack::~ScopedMemoryTrack() {
    if (m_address) {
        m_tracker.trackDeallocation(m_address);
        std::free(m_address);
    }
}

void* ScopedMemoryTrack::address() const {
    return m_address;
}

} }
