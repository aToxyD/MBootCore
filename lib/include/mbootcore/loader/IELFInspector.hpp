#pragma once

#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mbootcore {

struct ElfHeaderInfo {
    uint16_t type{0};
    uint16_t machine{0};
    uint32_t version{0};
    uint64_t entry{0};
    uint64_t phoff{0};
    uint64_t shoff{0};
    uint32_t flags{0};
    uint16_t ehsize{0};
    uint16_t phentsize{0};
    uint16_t phnum{0};
    uint16_t shentsize{0};
    uint16_t shnum{0};
    uint16_t shstrndx{0};
    bool is64Bit{false};
    bool isLittleEndian{true};
};

struct ElfSegmentInfo {
    uint32_t type{0};
    uint64_t offset{0};
    uint64_t vaddr{0};
    uint64_t paddr{0};
    uint64_t filesz{0};
    uint64_t memsz{0};
    uint32_t flags{0};
    uint64_t align{0};
};

struct ElfSectionInfo {
    std::string name;
    uint32_t type{0};
    uint64_t flags{0};
    uint64_t addr{0};
    uint64_t offset{0};
    uint64_t size{0};
    uint32_t link{0};
    uint32_t info{0};
    uint64_t addralign{0};
    uint64_t entsize{0};
};

struct ElfInspectionResult {
    ElfHeaderInfo header;
    std::vector<ElfSegmentInfo> segments;
    std::vector<ElfSectionInfo> sections;
    std::string architecture;
    bool hasProgramHeaders{false};
    bool hasSectionHeaders{false};
};

class IElfInspector {
public:
    virtual ~IElfInspector() = default;

    virtual Result<ElfInspectionResult> inspect(const ByteBuffer& data) = 0;
    virtual bool isElf(const ByteBuffer& data) const noexcept = 0;
    virtual bool is32Bit(const ByteBuffer& data) const noexcept = 0;
    virtual bool is64Bit(const ByteBuffer& data) const noexcept = 0;
    virtual uint64_t entryPoint(const ByteBuffer& data) const noexcept = 0;
    virtual std::string architecture(const ByteBuffer& data) const = 0;
    virtual std::vector<uint8_t> computeHash(const ByteBuffer& data) const = 0;
};

} // namespace mbootcore
