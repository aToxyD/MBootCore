#include "mbootcore/elf/ElfParser.hpp"

#include <cstring>
#include <algorithm>

namespace mbootcore {
namespace elf {

// Minimum entry sizes required by the parser's raw readers.
// These match the struct sizes read by readProgramHeader{32,64}
// and readSectionHeader{32,64}.
constexpr uint16_t kElf32ProgramHeaderSize = 32;
constexpr uint16_t kElf64ProgramHeaderSize = 56;
constexpr uint16_t kElf32SectionHeaderSize = 40;
constexpr uint16_t kElf64SectionHeaderSize = 64;

// ============================================================
// Endian-aware reads
// ============================================================

uint16_t ElfParser::readU16(const uint8_t* p, ElfEndian endian) noexcept {
    uint16_t val;
    std::memcpy(&val, p, sizeof(val));
    if (endian == ElfEndian::Big) {
        val = __builtin_bswap16(val);
    }
    return val;
}

uint32_t ElfParser::readU32(const uint8_t* p, ElfEndian endian) noexcept {
    uint32_t val;
    std::memcpy(&val, p, sizeof(val));
    if (endian == ElfEndian::Big) {
        val = __builtin_bswap32(val);
    }
    return val;
}

uint64_t ElfParser::readU64(const uint8_t* p, ElfEndian endian) noexcept {
    uint64_t val;
    std::memcpy(&val, p, sizeof(val));
    if (endian == ElfEndian::Big) {
        val = __builtin_bswap64(val);
    }
    return val;
}

// ============================================================
// Magic check
// ============================================================

bool ElfParser::hasValidMagic(const ByteBuffer& data) const noexcept {
    if (data.size() < 4) return false;
    return std::memcmp(data.data(), ELF_MAGIC, 4) == 0;
}

// ============================================================
// Quick checks
// ============================================================

bool ElfParser::isElf(const ByteBuffer& data) const noexcept {
    return hasValidMagic(data);
}

bool ElfParser::is32Bit(const ByteBuffer& data) const noexcept {
    return hasValidMagic(data) && data.size() >= 5 && data[4] == 1;
}

bool ElfParser::is64Bit(const ByteBuffer& data) const noexcept {
    return hasValidMagic(data) && data.size() >= 5 && data[4] == 2;
}

ElfClass ElfParser::identifyClass(const ByteBuffer& data) const noexcept {
    if (!hasValidMagic(data) || data.size() < 5) return ElfClass::None;
    return static_cast<ElfClass>(data[4]);
}

// ============================================================
// Static header readers
// ============================================================

ElfClass ElfParser::readClass(const ByteBuffer& data) noexcept {
    if (data.size() < 5) return ElfClass::None;
    return static_cast<ElfClass>(data[4]);
}

ElfEndian ElfParser::readEndian(const ByteBuffer& data) noexcept {
    if (data.size() < 6) return ElfEndian::None;
    return static_cast<ElfEndian>(data[5]);
}

uint16_t ElfParser::readMachine(const ByteBuffer& data) noexcept {
    if (data.size() < 20) return 0;
    auto endian = readEndian(data);
    return readU16(data.data() + 18, endian);
}

uint64_t ElfParser::readEntry(const ByteBuffer& data) noexcept {
    auto cls = readClass(data);
    auto endian = readEndian(data);
    if (cls == ElfClass::Elf32) {
        if (data.size() < 28) return 0;
        return readU32(data.data() + 24, endian);
    } else if (cls == ElfClass::Elf64) {
        if (data.size() < 32) return 0;
        return readU64(data.data() + 24, endian);
    }
    return 0;
}

ElfHeader ElfParser::readHeader(const ByteBuffer& data) noexcept {
    ElfHeader hdr;
    if (data.size() < 52) return hdr;

    hdr.elfClass = readClass(data);
    hdr.endian = readEndian(data);
    if (hdr.elfClass == ElfClass::None || hdr.endian == ElfEndian::None) return hdr;

    hdr.version = data[6];
    hdr.osAbi = static_cast<ElfOsAbi>(data[7]);
    hdr.abiVersion = data[8];

    const uint8_t* p = data.data();

    if (hdr.elfClass == ElfClass::Elf64 && data.size() >= 64) {
        hdr.type = static_cast<ElfType>(readU16(p + 16, hdr.endian));
        hdr.machine = static_cast<ElfMachine>(readU16(p + 18, hdr.endian));
        hdr.elfVersion = readU32(p + 20, hdr.endian);
        hdr.entry = readU64(p + 24, hdr.endian);
        hdr.phoff = readU64(p + 32, hdr.endian);
        hdr.shoff = readU64(p + 40, hdr.endian);
        hdr.flags = readU32(p + 48, hdr.endian);
        hdr.ehsize = readU16(p + 52, hdr.endian);
        hdr.phentsize = readU16(p + 54, hdr.endian);
        hdr.phnum = readU16(p + 56, hdr.endian);
        hdr.shentsize = readU16(p + 58, hdr.endian);
        hdr.shnum = readU16(p + 60, hdr.endian);
        hdr.shstrndx = readU16(p + 62, hdr.endian);
    } else if (hdr.elfClass == ElfClass::Elf32 && data.size() >= 52) {
        hdr.type = static_cast<ElfType>(readU16(p + 16, hdr.endian));
        hdr.machine = static_cast<ElfMachine>(readU16(p + 18, hdr.endian));
        hdr.elfVersion = readU32(p + 20, hdr.endian);
        hdr.entry = readU32(p + 24, hdr.endian);
        hdr.phoff = readU32(p + 28, hdr.endian);
        hdr.shoff = readU32(p + 32, hdr.endian);
        hdr.flags = readU32(p + 36, hdr.endian);
        hdr.ehsize = readU16(p + 40, hdr.endian);
        hdr.phentsize = readU16(p + 42, hdr.endian);
        hdr.phnum = readU16(p + 44, hdr.endian);
        hdr.shentsize = readU16(p + 46, hdr.endian);
        hdr.shnum = readU16(p + 48, hdr.endian);
        hdr.shstrndx = readU16(p + 50, hdr.endian);
    }

    return hdr;
}

// ============================================================
// Program header readers
// ============================================================

ProgramHeader ElfParser::readProgramHeader32(const uint8_t* p, ElfEndian endian) noexcept {
    ProgramHeader ph;
    ph.type = static_cast<SegmentType>(readU32(p + 0, endian));
    ph.offset = readU32(p + 4, endian);
    ph.vaddr = readU32(p + 8, endian);
    ph.paddr = readU32(p + 12, endian);
    ph.filesz = readU32(p + 16, endian);
    ph.memsz = readU32(p + 20, endian);
    ph.flags = readU32(p + 24, endian);
    ph.align = readU32(p + 28, endian);
    return ph;
}

ProgramHeader ElfParser::readProgramHeader64(const uint8_t* p, ElfEndian endian) noexcept {
    ProgramHeader ph;
    ph.type = static_cast<SegmentType>(readU32(p + 0, endian));
    ph.flags = readU32(p + 4, endian);
    ph.offset = readU64(p + 8, endian);
    ph.vaddr = readU64(p + 16, endian);
    ph.paddr = readU64(p + 24, endian);
    ph.filesz = readU64(p + 32, endian);
    ph.memsz = readU64(p + 40, endian);
    ph.align = readU64(p + 48, endian);
    return ph;
}

SectionHeader ElfParser::readSectionHeader32(const uint8_t* p, ElfEndian endian) noexcept {
    SectionHeader sh;
    sh.nameIdx = readU32(p + 0, endian);
    sh.type = static_cast<SectionType>(readU32(p + 4, endian));
    sh.flags = readU32(p + 8, endian);
    sh.addr = readU32(p + 12, endian);
    sh.offset = readU32(p + 16, endian);
    sh.size = readU32(p + 20, endian);
    sh.link = readU32(p + 24, endian);
    sh.info = readU32(p + 28, endian);
    sh.addralign = readU32(p + 32, endian);
    sh.entsize = readU32(p + 36, endian);
    return sh;
}

SectionHeader ElfParser::readSectionHeader64(const uint8_t* p, ElfEndian endian) noexcept {
    SectionHeader sh;
    sh.nameIdx = readU32(p + 0, endian);
    sh.type = static_cast<SectionType>(readU32(p + 4, endian));
    sh.flags = readU64(p + 8, endian);
    sh.addr = readU64(p + 16, endian);
    sh.offset = readU64(p + 24, endian);
    sh.size = readU64(p + 32, endian);
    sh.link = readU32(p + 40, endian);
    sh.info = readU32(p + 44, endian);
    sh.addralign = readU64(p + 48, endian);
    sh.entsize = readU64(p + 56, endian);
    return sh;
}

// ============================================================
// String table reader
// ============================================================

std::string ElfParser::readStringTable(const ByteBuffer& data, uint64_t offset,
                                        size_t size, ElfEndian endian) const noexcept {
    (void)endian;
    if (offset >= data.size() || size == 0) return {};
    auto available = data.size() - static_cast<size_t>(offset);
    size_t readSize = std::min(static_cast<size_t>(size), available);
    return std::string(reinterpret_cast<const char*>(data.data() + offset), readSize);
}

// ============================================================
// Validation helpers
// ============================================================

bool ElfParser::validateHeader(const ElfHeader& hdr, size_t dataSize, ElfValidation& v) {
    if (hdr.elfClass == ElfClass::None) {
        v.addError("Invalid or unknown ELF class");
        return false;
    }
    if (hdr.endian == ElfEndian::None) {
        v.addError("Invalid or unknown endianness");
        return false;
    }
    if (hdr.elfVersion != 1) {
        v.addError("Invalid ELF version (expected 1)");
        return false;
    }
    if (hdr.type != ElfType::Exec && hdr.type != ElfType::Dyn && hdr.type != ElfType::Rel) {
        v.addWarning("Unusual ELF type: " + std::to_string(static_cast<uint16_t>(hdr.type)));
    }
    if (hdr.machine == ElfMachine::None) {
        v.addError("No machine specified");
        return false;
    }
    if (hdr.phnum > 0) {
        const uint16_t minPhEnt = (hdr.elfClass == ElfClass::Elf64)
                                      ? kElf64ProgramHeaderSize : kElf32ProgramHeaderSize;
        if (hdr.phentsize < minPhEnt) {
            v.addError("Program header entry size below minimum");
            return false;
        }
    }
    if (hdr.shnum > 0) {
        const uint16_t minShEnt = (hdr.elfClass == ElfClass::Elf64)
                                      ? kElf64SectionHeaderSize : kElf32SectionHeaderSize;
        if (hdr.shentsize < minShEnt) {
            v.addError("Section header entry size below minimum");
            return false;
        }
    }
    if (hdr.entry == 0) {
        v.addWarning("Entry point is 0");
    }
    if (hdr.ehsize > 0 && hdr.ehsize != (hdr.elfClass == ElfClass::Elf64 ? 64 : 52)) {
        v.addWarning("Unusual ELF header size: " + std::to_string(hdr.ehsize));
    }
    if (hdr.phoff > 0 && hdr.phoff < static_cast<uint64_t>(hdr.ehsize > 0 ? hdr.ehsize : 52)) {
        v.addError("Program header offset overlaps with ELF header");
        return false;
    }
    if (hdr.phoff > 0 && hdr.phoff + static_cast<uint64_t>(hdr.phnum) * hdr.phentsize > dataSize) {
        v.addError("Program headers extend beyond file");
        return false;
    }
    if (hdr.shoff > 0 && hdr.shoff + static_cast<uint64_t>(hdr.shnum) * hdr.shentsize > dataSize) {
        v.addError("Section headers extend beyond file");
        return false;
    }
    return true;
}

bool ElfParser::validateProgramHeaders(const ElfHeader& hdr, const ByteBuffer& data,
                                        size_t /*phStart*/,
                                        const std::vector<ProgramHeader>& phs,
                                        ElfValidation& v) {
    (void)hdr;
    bool hasLoadable = false;

    for (size_t i = 0; i < phs.size(); ++i) {
        const auto& ph = phs[i];

        if (ph.type == SegmentType::Null) continue;

        if (ph.isLoadable()) hasLoadable = true;

        if (ph.filesz > data.size() || ph.offset > data.size() - ph.filesz) {
            v.addError("Segment " + std::to_string(i) +
                       " (file data) extends beyond file bounds");
        }

        if (ph.filesz > ph.memsz) {
            v.addWarning("Segment " + std::to_string(i) +
                         " filesz > memsz (truncated in memory)");
        }

        if (ph.align > 1 && (ph.vaddr % ph.align) != 0) {
            v.addWarning("Segment " + std::to_string(i) +
                         " vaddr not aligned to p_align");
        }

        if (ph.align > 1 && (ph.offset % ph.align) != 0) {
            v.addWarning("Segment " + std::to_string(i) +
                         " offset not aligned to p_align");
        }

        if (ph.type == SegmentType::Load && ph.filesz == 0 && ph.memsz > 0) {
            v.addWarning("Segment " + std::to_string(i) + " BSS-only (filesz=0, memsz>0)");
        }
    }

    // Check overlapping segments
    for (size_t i = 0; i < phs.size(); ++i) {
        if (!phs[i].isLoadable()) continue;
        for (size_t j = i + 1; j < phs.size(); ++j) {
            if (!phs[j].isLoadable()) continue;
            if (phs[i].vaddr < phs[j].endVaddr() &&
                phs[j].vaddr < phs[i].endVaddr()) {
                v.addError("Overlapping PT_LOAD segments " +
                           std::to_string(i) + " and " + std::to_string(j));
            }
        }
    }

    if (!hasLoadable && phs.size() > 0) {
        v.addWarning("No PT_LOAD segments found");
    }

    return v.valid;
}

bool ElfParser::validateSectionHeaders(const ElfHeader& /*hdr*/, const ByteBuffer& /*data*/,
                                        size_t /*shStart*/, ElfValidation& /*v*/) {
    return true;
}

// ============================================================
// Main parse
// ============================================================

Result<ElfFile> ElfParser::parse(const ByteBuffer& data) {
    ElfFile elf;

    if (!hasValidMagic(data)) {
        return ErrorCode::InvalidElf;
    }

    elf.header = readHeader(data);
    elf.rawData = data;

    // Validate header
    if (!validateHeader(elf.header, data.size(), elf.validation)) {
        return ErrorCode::InvalidElf;
    }

    auto endian = elf.header.endian;
    size_t phSize = elf.header.phentsize > 0
                    ? static_cast<size_t>(elf.header.phentsize) : 0;
    size_t shSize = elf.header.shentsize > 0
                    ? static_cast<size_t>(elf.header.shentsize) : 0;
    const size_t expectedPhSize = (elf.header.elfClass == ElfClass::Elf64)
                                      ? kElf64ProgramHeaderSize : kElf32ProgramHeaderSize;
    const size_t expectedShSize = (elf.header.elfClass == ElfClass::Elf64)
                                      ? kElf64SectionHeaderSize : kElf32SectionHeaderSize;

    // Parse program headers
    if (elf.header.phnum > 0 && phSize > 0 &&
        elf.header.phoff > 0 && elf.header.phoff < data.size()) {
        elf.programHeaders.reserve(elf.header.phnum);
        for (uint16_t i = 0; i < elf.header.phnum; ++i) {
            uint64_t off = elf.header.phoff + static_cast<uint64_t>(i) * phSize;
            if (off + expectedPhSize > data.size()) break;

            ProgramHeader ph;
            if (elf.header.elfClass == ElfClass::Elf64) {
                ph = readProgramHeader64(data.data() + off, endian);
            } else {
                ph = readProgramHeader32(data.data() + off, endian);
            }
            elf.programHeaders.push_back(std::move(ph));
        }

        validateProgramHeaders(elf.header, data,
                               static_cast<size_t>(elf.header.phoff),
                               elf.programHeaders, elf.validation);
    }

    // Parse section headers
    if (elf.header.shnum > 0 && shSize > 0 &&
        elf.header.shoff > 0 && elf.header.shoff < data.size()) {
        elf.sectionHeaders.reserve(elf.header.shnum);
        for (uint16_t i = 0; i < elf.header.shnum; ++i) {
            uint64_t off = elf.header.shoff + static_cast<uint64_t>(i) * shSize;
            if (off + expectedShSize > data.size()) break;

            SectionHeader sh;
            if (elf.header.elfClass == ElfClass::Elf64) {
                sh = readSectionHeader64(data.data() + off, endian);
            } else {
                sh = readSectionHeader32(data.data() + off, endian);
            }
            elf.sectionHeaders.push_back(std::move(sh));
        }

        // Resolve section names from string table
        if (elf.header.shstrndx < elf.sectionHeaders.size()) {
            const auto& strtabHdr = elf.sectionHeaders[elf.header.shstrndx];
            if (strtabHdr.type == SectionType::StrTab &&
                strtabHdr.offset < data.size()) {
                size_t strSize = static_cast<size_t>(std::min(strtabHdr.size,
                    static_cast<uint64_t>(data.size() - strtabHdr.offset)));
                auto strData = readStringTable(data, strtabHdr.offset,
                                                strSize, endian);

                for (auto& sh : elf.sectionHeaders) {
                    if (sh.nameIdx < strData.size()) {
                        auto end = strData.find('\0', sh.nameIdx);
                        sh.name = strData.substr(sh.nameIdx,
                                                  end - sh.nameIdx);
                    }
                }
            }
        }

        validateSectionHeaders(elf.header, data,
                                static_cast<size_t>(elf.header.shoff),
                                elf.validation);
    }

    return elf;
}

} // namespace elf
} // namespace mbootcore
