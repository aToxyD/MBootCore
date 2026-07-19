#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <chrono>

#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace memory {

struct AllocationRecord {
    void* address{nullptr};
    size_t size{0};
    std::string label;
    std::chrono::steady_clock::time_point timestamp;
    bool freed{false};
};

struct MemoryReport {
    size_t currentUsage{0};
    size_t peakUsage{0};
    uint64_t totalAllocations{0};
    uint64_t totalDeallocations{0};
    size_t liveAllocations{0};
    size_t largeBufferThreshold{1024 * 1024};
    std::vector<AllocationRecord> liveObjects;
    std::vector<AllocationRecord> largeBuffers;
    std::chrono::steady_clock::time_point timestamp;
};

class MemoryTracker {
public:
    MemoryTracker();
    ~MemoryTracker();
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;
    MemoryTracker(MemoryTracker&&) noexcept;
    MemoryTracker& operator=(MemoryTracker&&) noexcept;

    void trackAllocation(void* address, size_t size, const std::string& label = "");
    void trackDeallocation(void* address);

    Result<MemoryReport> report() const;
    Result<MemoryReport> snapshot() const;
    Result<std::string> exportReport() const;

    size_t currentUsage() const;
    size_t peakUsage() const;
    uint64_t totalAllocations() const;
    uint64_t totalDeallocations() const;
    size_t liveAllocationCount() const;

    void setLargeBufferThreshold(size_t bytes);
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class ScopedMemoryTrack {
public:
    ScopedMemoryTrack(MemoryTracker& tracker, size_t size, const std::string& label = "");
    ~ScopedMemoryTrack();
    void* address() const;
private:
    MemoryTracker& m_tracker;
    void* m_address{nullptr};
    size_t m_size{0};
};

} }
