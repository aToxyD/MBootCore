#pragma once

#include "mbootcore/elf/ElfModels.hpp"

namespace mbootcore {
namespace elf {

class ElfValidator {
public:
    ElfValidator() = default;

    ElfValidation validate(const ElfFile& elf, const ByteBuffer& rawData);

    // Individual checks
    bool checkMagic(const ByteBuffer& data) noexcept;
    bool checkVersion(const ElfHeader& hdr) noexcept;
    bool checkMachine(const ElfHeader& hdr) noexcept;
    bool checkEntryPoint(const ElfHeader& hdr) noexcept;
    bool checkOffsets(const ElfHeader& hdr, size_t dataSize) noexcept;
    bool checkAlignment(const std::vector<ProgramHeader>& phs) noexcept;

    // Segment overlap check (most critical for memory loading)
    bool checkOverlappingSegments(const std::vector<ProgramHeader>& phs) noexcept;

    // Truncation checks
    bool checkNotTruncated(const ElfHeader& hdr, size_t dataSize) noexcept;

    // Unsupported type check
    bool checkSupportedType(const ElfHeader& hdr) noexcept;

private:
    bool areSegmentsOverlapping(const ProgramHeader& a, const ProgramHeader& b) const noexcept;
};

} // namespace elf
} // namespace mbootcore
