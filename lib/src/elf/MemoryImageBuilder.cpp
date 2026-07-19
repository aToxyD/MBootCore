#include "mbootcore/elf/MemoryImageBuilder.hpp"

#include <algorithm>
#include <cstring>

namespace mbootcore {
namespace elf {

uint64_t MemoryImageBuilder::alignUp(uint64_t addr) const noexcept {
    if (m_alignment == 0) return addr;
    return (addr + m_alignment - 1) & ~(m_alignment - 1);
}

uint64_t MemoryImageBuilder::alignDown(uint64_t addr) const noexcept {
    if (m_alignment == 0) return addr;
    return addr & ~(m_alignment - 1);
}

bool MemoryImageBuilder::isSegmentRequired(const ProgramHeader& ph) const noexcept {
    if (!ph.isLoadable()) return false;
    if (m_requiredFlags != 0) {
        if ((ph.flags & m_requiredFlags) != m_requiredFlags) return false;
    }
    return true;
}

Result<MemoryImage> MemoryImageBuilder::build(const ElfFile& elf) {
    MemoryImage mem;

    if (!elf.isValid()) {
        return ErrorCode::InvalidElf;
    }

    // Collect loadable segments
    const size_t rawSize = elf.rawData.size();
    std::vector<SegmentData> segments;
    for (const auto& ph : elf.programHeaders) {
        if (!isSegmentRequired(ph)) continue;

        SegmentData seg;
        seg.vaddr = ph.vaddr;
        seg.paddr = ph.paddr;
        seg.filesz = ph.filesz;
        seg.memsz = ph.memsz;
        seg.flags = ph.flags;

        // Copy file data for this segment.
        // Overflow-safe: filesz <= rawSize guards the subtraction below.
        if (ph.filesz > 0 &&
            ph.filesz <= rawSize &&
            ph.offset <= rawSize - ph.filesz) {
            seg.data.assign(
                elf.rawData.begin() + static_cast<ptrdiff_t>(ph.offset),
                elf.rawData.begin() + static_cast<ptrdiff_t>(ph.offset + ph.filesz));
        }

        // Pad to memsz if filesz < memsz (BSS)
        if (seg.data.size() < seg.memsz) {
            seg.data.resize(static_cast<size_t>(seg.memsz), 0);
        }

        segments.push_back(std::move(seg));
    }

    if (segments.empty()) {
        return ErrorCode::InvalidElf;
    }

    // Sort by virtual address
    if (m_sortByVaddr) {
        std::sort(segments.begin(), segments.end(),
            [](const SegmentData& a, const SegmentData& b) {
                return a.vaddr < b.vaddr;
            });
    }

    // Compute memory ranges
    mem.lowestAddr = segments.front().vaddr;
    mem.highestAddr = 0;
    mem.totalFileSize = 0;
    mem.totalMemSize = 0;

    for (const auto& seg : segments) {
        auto end = seg.vaddr + seg.memsz;
        if (end > mem.highestAddr) mem.highestAddr = end;
        mem.totalFileSize += seg.filesz;
        mem.totalMemSize += seg.memsz;
    }

    mem.entryPoint = elf.header.entry;
    mem.segments = std::move(segments);

    // Build contiguous image
    if (mem.segments.size() == 1) {
        mem.image = mem.segments[0].data;
    } else {
        // Merge segments into contiguous buffer
        uint64_t totalSize = mem.highestAddr - mem.lowestAddr;
        mem.image.resize(static_cast<size_t>(totalSize), 0);

        for (const auto& seg : mem.segments) {
            uint64_t offset = seg.vaddr - mem.lowestAddr;
            if (offset + seg.data.size() <= totalSize) {
                std::memcpy(mem.image.data() + offset,
                            seg.data.data(), seg.data.size());
            }
        }
    }

    return mem;
}

} // namespace elf
} // namespace mbootcore
