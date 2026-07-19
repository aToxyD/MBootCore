#pragma once

#include "mbootcore/elf/ElfModels.hpp"
#include "mbootcore/domain/Error.hpp"

namespace mbootcore {
namespace elf {

class MemoryImageBuilder {
public:
    MemoryImageBuilder() = default;

    Result<MemoryImage> build(const ElfFile& elf);

    // Configure alignment requirements (default: 4K)
    void setAlignment(uint64_t align) noexcept { m_alignment = align; }
    uint64_t alignment() const noexcept { return m_alignment; }

    // Include only segments with this flag combination
    void setRequiredFlags(uint32_t flags) noexcept { m_requiredFlags = flags; }
    uint32_t requiredFlags() const noexcept { return m_requiredFlags; }

    // Segment sorting options
    void setSortByVaddr(bool sort) noexcept { m_sortByVaddr = sort; }
    bool sortByVaddr() const noexcept { return m_sortByVaddr; }

private:
    uint64_t m_alignment{4096};
    uint32_t m_requiredFlags{0};
    bool m_sortByVaddr{true};

    bool isSegmentRequired(const ProgramHeader& ph) const noexcept;
    uint64_t alignUp(uint64_t addr) const noexcept;
    uint64_t alignDown(uint64_t addr) const noexcept;
};

} // namespace elf
} // namespace mbootcore
