#include "mbootcore/elf/ElfValidator.hpp"

#include <cstring>

namespace mbootcore {
namespace elf {

ElfValidation ElfValidator::validate(const ElfFile& elf, const ByteBuffer& rawData) {
    ElfValidation v;

    if (!checkMagic(rawData)) {
        v.addError("Invalid ELF magic");
        return v;
    }

    if (!checkVersion(elf.header)) {
        v.addError("Invalid ELF version");
    }

    if (!checkMachine(elf.header)) {
        v.addError("Invalid or unsupported machine type");
    }

    if (!checkEntryPoint(elf.header)) {
        v.addWarning("Entry point is zero");
    }

    if (!checkOffsets(elf.header, rawData.size())) {
        v.addError("Invalid or out-of-bounds offsets");
    }

    if (!checkNotTruncated(elf.header, rawData.size())) {
        v.addError("File appears truncated");
    }

    if (!checkSupportedType(elf.header)) {
        v.addWarning("Unusual ELF type (not EXEC or DYN)");
    }

    if (!checkOverlappingSegments(elf.programHeaders)) {
        v.addError("Overlapping PT_LOAD segments detected");
    }

    if (!checkAlignment(elf.programHeaders)) {
        v.addWarning("Segment alignment issues detected");
    }

    return v;
}

bool ElfValidator::checkMagic(const ByteBuffer& data) noexcept {
    if (data.size() < 4) return false;
    return std::memcmp(data.data(), "\x7F""ELF", 4) == 0;
}

bool ElfValidator::checkVersion(const ElfHeader& hdr) noexcept {
    return hdr.elfVersion == 1;
}

bool ElfValidator::checkMachine(const ElfHeader& hdr) noexcept {
    return hdr.machine != ElfMachine::None;
}

bool ElfValidator::checkEntryPoint(const ElfHeader& hdr) noexcept {
    return hdr.entry != 0;
}

bool ElfValidator::checkOffsets(const ElfHeader& hdr, size_t dataSize) noexcept {
    if (hdr.phoff > 0) {
        uint64_t phEnd = hdr.phoff + static_cast<uint64_t>(hdr.phnum) * hdr.phentsize;
        if (phEnd > dataSize) return false;
    }
    if (hdr.shoff > 0) {
        uint64_t shEnd = hdr.shoff + static_cast<uint64_t>(hdr.shnum) * hdr.shentsize;
        if (shEnd > dataSize) return false;
    }
    return true;
}

bool ElfValidator::checkNotTruncated(const ElfHeader& hdr, size_t dataSize) noexcept {
    if (hdr.phnum > 0) {
        uint64_t phEnd = hdr.phoff + static_cast<uint64_t>(hdr.phnum) * hdr.phentsize;
        if (phEnd > dataSize) return false;
    }
    if (hdr.shnum > 0) {
        uint64_t shEnd = hdr.shoff + static_cast<uint64_t>(hdr.shnum) * hdr.shentsize;
        if (shEnd > dataSize) return false;
    }
    return true;
}

bool ElfValidator::checkSupportedType(const ElfHeader& hdr) noexcept {
    return hdr.type == ElfType::Exec || hdr.type == ElfType::Dyn || hdr.type == ElfType::Rel;
}

bool ElfValidator::areSegmentsOverlapping(const ProgramHeader& a,
                                           const ProgramHeader& b) const noexcept {
    if (!a.isLoadable() || !b.isLoadable()) return false;
    return a.vaddr < b.endVaddr() && b.vaddr < a.endVaddr();
}

bool ElfValidator::checkOverlappingSegments(const std::vector<ProgramHeader>& phs) noexcept {
    for (size_t i = 0; i < phs.size(); ++i) {
        if (!phs[i].isLoadable()) continue;
        for (size_t j = i + 1; j < phs.size(); ++j) {
            if (!phs[j].isLoadable()) continue;
            if (areSegmentsOverlapping(phs[i], phs[j])) return false;
        }
    }
    return true;
}

bool ElfValidator::checkAlignment(const std::vector<ProgramHeader>& phs) noexcept {
    bool allOk = true;
    for (const auto& ph : phs) {
        if (!ph.isLoadable()) continue;
        if (ph.align > 1) {
            if ((ph.vaddr % ph.align) != 0) allOk = false;
            if ((ph.offset % ph.align) != 0) allOk = false;
        }
    }
    return allOk;
}

} // namespace elf
} // namespace mbootcore
