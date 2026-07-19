#pragma once

#include "mbootcore/elf/ElfTypes.hpp"
#include "mbootcore/domain/Types.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace mbootcore {
namespace elf {

// ============================================================
// ELF Header — strongly typed
// ============================================================
struct ElfHeader {
    ElfClass    elfClass{ElfClass::None};
    ElfEndian   endian{ElfEndian::None};
    uint8_t     version{0};
    ElfOsAbi    osAbi{ElfOsAbi::SystemV};
    uint8_t     abiVersion{0};
    ElfType     type{ElfType::None};
    ElfMachine  machine{ElfMachine::None};
    uint32_t    elfVersion{0};
    uint64_t    entry{0};
    uint64_t    phoff{0};
    uint64_t    shoff{0};
    uint32_t    flags{0};
    uint16_t    ehsize{0};
    uint16_t    phentsize{0};
    uint16_t    phnum{0};
    uint16_t    shentsize{0};
    uint16_t    shnum{0};
    uint16_t    shstrndx{0};

    bool isValid() const noexcept {
        return elfClass != ElfClass::None && elfVersion == 1;
    }
};

// ============================================================
// Program Header
// ============================================================
struct ProgramHeader {
    SegmentType type{SegmentType::Null};
    uint32_t    flags{0};
    uint64_t    offset{0};
    uint64_t    vaddr{0};
    uint64_t    paddr{0};
    uint64_t    filesz{0};
    uint64_t    memsz{0};
    uint64_t    align{0};

    bool isLoadable() const noexcept {
        return type == SegmentType::Load;
    }

    bool isReadable() const noexcept {
        return (flags & static_cast<uint32_t>(SegmentFlag::R)) != 0;
    }

    bool isWritable() const noexcept {
        return (flags & static_cast<uint32_t>(SegmentFlag::W)) != 0;
    }

    bool isExecutable() const noexcept {
        return (flags & static_cast<uint32_t>(SegmentFlag::X)) != 0;
    }

    uint64_t endOffset() const noexcept {
        return offset + filesz;
    }

    uint64_t endVaddr() const noexcept {
        return vaddr + memsz;
    }

    bool containsOffset(uint64_t off) const noexcept {
        return off >= offset && off < offset + filesz;
    }

    bool containsVaddr(uint64_t addr) const noexcept {
        return addr >= vaddr && addr < vaddr + memsz;
    }
};

// ============================================================
// Section Header
// ============================================================
struct SectionHeader {
    std::string name;
    uint32_t    nameIdx{0};
    SectionType type{SectionType::Null};
    uint64_t    flags{0};
    uint64_t    addr{0};
    uint64_t    offset{0};
    uint64_t    size{0};
    uint32_t    link{0};
    uint32_t    info{0};
    uint64_t    addralign{0};
    uint64_t    entsize{0};

    bool isAlloc() const noexcept {
        return (flags & static_cast<uint64_t>(SectionFlag::Alloc)) != 0;
    }

    bool isExecInstr() const noexcept {
        return (flags & static_cast<uint64_t>(SectionFlag::ExecInstr)) != 0;
    }

    bool isWritable() const noexcept {
        return (flags & static_cast<uint64_t>(SectionFlag::Write)) != 0;
    }
};

// ============================================================
// Segment Data — extracted raw bytes from a PT_LOAD segment
// ============================================================
struct SegmentData {
    uint64_t    vaddr{0};
    uint64_t    paddr{0};
    uint64_t    filesz{0};
    uint64_t    memsz{0};
    uint32_t    flags{0};
    ByteBuffer  data;

    uint64_t endVaddr() const noexcept { return vaddr + memsz; }

    bool contains(uint64_t addr) const noexcept {
        return addr >= vaddr && addr < vaddr + memsz;
    }
};

// ============================================================
// Memory Image — built from loadable segments
// ============================================================
struct MemoryImage {
    uint64_t    lowestAddr{0};
    uint64_t    highestAddr{0};
    uint64_t    totalFileSize{0};
    uint64_t    totalMemSize{0};
    uint64_t    entryPoint{0};
    ByteBuffer  image;
    std::vector<SegmentData> segments;

    bool isEmpty() const noexcept { return segments.empty(); }
};

// ============================================================
// Validation Result
// ============================================================
struct ElfValidation {
    bool        valid{true};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    void addWarning(std::string msg) { warnings.push_back(std::move(msg)); }
    void addError(std::string msg) {
        errors.push_back(std::move(msg));
        valid = false;
    }
};

// ============================================================
// Parsed ELF File — complete model
// ============================================================
struct ElfFile {
    ElfHeader               header;
    std::vector<ProgramHeader>  programHeaders;
    std::vector<SectionHeader>  sectionHeaders;
    ByteBuffer              rawData;
    ElfValidation           validation;

    bool isValid() const noexcept { return validation.valid && header.isValid(); }

    const ProgramHeader* findSegment(SegmentType type) const noexcept {
        for (const auto& ph : programHeaders) {
            if (ph.type == type) return &ph;
        }
        return nullptr;
    }

    std::vector<const ProgramHeader*> loadableSegments() const noexcept {
        std::vector<const ProgramHeader*> result;
        for (const auto& ph : programHeaders) {
            if (ph.isLoadable()) result.push_back(&ph);
        }
        return result;
    }

    const SectionHeader* findSection(const std::string& name) const noexcept {
        for (const auto& sh : sectionHeaders) {
            if (sh.name == name) return &sh;
        }
        return nullptr;
    }

    std::string machineName() const noexcept {
        return machineToString(header.machine);
    }
};

} // namespace elf
} // namespace mbootcore
