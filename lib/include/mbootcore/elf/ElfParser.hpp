#pragma once

#include "mbootcore/elf/ElfModels.hpp"
#include "mbootcore/domain/Error.hpp"

#include <memory>

namespace mbootcore {
namespace elf {

class ElfParser {
public:
    ElfParser() = default;

    Result<ElfFile> parse(const ByteBuffer& data);

    // Quick checks
    bool isElf(const ByteBuffer& data) const noexcept;
    bool is32Bit(const ByteBuffer& data) const noexcept;
    bool is64Bit(const ByteBuffer& data) const noexcept;
    ElfClass identifyClass(const ByteBuffer& data) const noexcept;

    // Raw header read helpers (no validation, noexcept)
    static ElfHeader readHeader(const ByteBuffer& data) noexcept;
    static ElfClass readClass(const ByteBuffer& data) noexcept;
    static ElfEndian readEndian(const ByteBuffer& data) noexcept;
    static uint16_t readMachine(const ByteBuffer& data) noexcept;
    static uint64_t readEntry(const ByteBuffer& data) noexcept;

private:
    static constexpr uint8_t ELF_MAGIC[4] = {0x7F, 'E', 'L', 'F'};

    bool hasValidMagic(const ByteBuffer& data) const noexcept;

    bool validateHeader(const ElfHeader& hdr, size_t dataSize, ElfValidation& v);
    bool validateProgramHeaders(const ElfHeader& hdr, const ByteBuffer& data,
                                size_t phStart, const std::vector<ProgramHeader>& phs,
                                ElfValidation& v);
    bool validateSectionHeaders(const ElfHeader& hdr, const ByteBuffer& data,
                                size_t shStart, ElfValidation& v);

    static uint16_t readU16(const uint8_t* p, ElfEndian endian) noexcept;
    static uint32_t readU32(const uint8_t* p, ElfEndian endian) noexcept;
    static uint64_t readU64(const uint8_t* p, ElfEndian endian) noexcept;

    static ProgramHeader readProgramHeader32(const uint8_t* p, ElfEndian endian) noexcept;
    static ProgramHeader readProgramHeader64(const uint8_t* p, ElfEndian endian) noexcept;
    static SectionHeader readSectionHeader32(const uint8_t* p, ElfEndian endian) noexcept;
    static SectionHeader readSectionHeader64(const uint8_t* p, ElfEndian endian) noexcept;

    std::string readStringTable(const ByteBuffer& data, uint64_t offset,
                                 size_t size, ElfEndian endian) const noexcept;
};

} // namespace elf
} // namespace mbootcore
