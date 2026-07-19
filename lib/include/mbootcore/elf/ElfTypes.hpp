#pragma once

#include <cstdint>
#include <cstddef>

namespace mbootcore {
namespace elf {

// ============================================================
// ELF Magic
// ============================================================
inline constexpr uint8_t  ELFMAG0 = 0x7F;
inline constexpr uint8_t  ELFMAG1 = 'E';
inline constexpr uint8_t  ELFMAG2 = 'L';
inline constexpr uint8_t  ELFMAG3 = 'F';
inline constexpr uint32_t ELF_MAGIC = 0x464C457F;  // "\x7FELF" as LE uint32

// ============================================================
// ELF Class (32-bit vs 64-bit)
// ============================================================
enum class ElfClass : uint8_t {
    None  = 0,
    Elf32 = 1,
    Elf64 = 2
};

// ============================================================
// Endianness
// ============================================================
enum class ElfEndian : uint8_t {
    None    = 0,
    Little  = 1,
    Big     = 2
};

// ============================================================
// ELF OS/ABI
// ============================================================
enum class ElfOsAbi : uint8_t {
    SystemV      = 0,
    HPUX         = 1,
    NetBSD       = 2,
    Linux        = 3,
    GnuHurd      = 4,
    Solaris      = 6,
    AIX          = 7,
    IRIX         = 8,
    FreeBSD      = 9,
    Tru64        = 10,
    Modesto      = 11,
    OpenBSD      = 12,
    OpenVMS      = 13,
    NonStopKernel= 14,
    AROS         = 15,
    FenixOS      = 16,
    CloudABI     = 17,
    OpenVOS      = 18
};

// ============================================================
// ELF Type (e_type)
// ============================================================
enum class ElfType : uint16_t {
    None       = 0,
    Rel        = 1,
    Exec       = 2,
    Dyn        = 3,
    Core       = 4,
    LoOS       = 0xFE00,
    HiOS       = 0xFEFF,
    LoProc     = 0xFF00,
    HiProc     = 0xFFFF
};

// ============================================================
// Machine (e_machine)
// ============================================================
enum class ElfMachine : uint16_t {
    None       = 0,
    M32        = 1,
    SPARC      = 2,
    x86        = 3,
    M68K       = 4,
    M88K       = 5,
    IAMCU      = 6,
    i860       = 7,
    MIPS       = 8,
    S370       = 9,
    MIPS_RS3_LE= 10,
    PARISC     = 15,
    VPP500     = 17,
    SPARC32PLUS= 18,
    i960       = 19,
    PPC        = 20,
    PPC64      = 21,
    S390       = 22,
    SPU        = 23,
    V800       = 36,
    FR20       = 37,
    RH32       = 38,
    RCE        = 39,
    ARM        = 40,
    Alpha      = 41,
    SH         = 42,
    SPARCV9    = 43,
    TRICORE    = 44,
    ARC        = 45,
    H8_300     = 46,
    H8_300H    = 47,
    H8S        = 48,
    H8_500     = 49,
    IA_64      = 50,
    MIPS_X     = 51,
    ColdFire   = 52,
    M68HC12    = 53,
    MMA        = 54,
    PCP        = 55,
    NCPU       = 56,
    NDR1       = 57,
    STARCORE   = 58,
    ME16       = 59,
    ST100      = 60,
    TINYJ      = 61,
    x86_64     = 62,
    PDSP       = 63,
    PDP10      = 64,
    PDP11      = 65,
    FX66       = 66,
    ST9PLUS    = 67,
    ST7        = 68,
    M68HC16    = 69,
    M68HC11    = 70,
    M68HC08    = 71,
    M68HC05    = 72,
    SVX        = 73,
    ST19       = 74,
    VAX        = 75,
    CRIS       = 76,
    JAVELIN    = 77,
    FIREPATH   = 78,
    ZSP        = 79,
    MMIX       = 80,
    HUANY      = 81,
    PRISM      = 82,
    AVR        = 83,
    FR30       = 84,
    D10V       = 85,
    D30V       = 86,
    V850       = 87,
    M32R       = 88,
    MN10300    = 89,
    MN10200    = 90,
    PJ         = 91,
    OpenRISC   = 92,
    ARC_COMPACT= 93,
    XTENSA     = 94,
    VIDEOCORE  = 95,
    TMM_GPP    = 96,
    NS32K      = 97,
    TPC        = 98,
    SNP1K      = 99,
    ST200      = 100,
    IP2K       = 101,
    MAX        = 102,
    CR         = 103,
    F2MC16     = 104,
    MSP430     = 105,
    BLACKFIN   = 106,
    SE_C33     = 107,
    SEP        = 108,
    ARCA       = 109,
    UNICOORE   = 110,
    AARCH64    = 183,
    RISC_V     = 243,
    BPF        = 247,
    WDC65C816  = 0x101
};

// ============================================================
// Program Header Types (p_type)
// ============================================================
enum class SegmentType : uint32_t {
    Null        = 0,
    Load        = 1,
    Dynamic     = 2,
    Interp      = 3,
    Note        = 4,
    ShLib       = 5,
    Phdr        = 6,
    TLS         = 7,
    LoOS        = 0x60000000,
    HiOS        = 0x6FFFFFFF,
    LoProc      = 0x70000000,
    HiProc      = 0x7FFFFFFF,

    // Qualcomm-specific (used in Firehose programmers)
    QDSS6       = 0x70000001,  // Hexagon DSP segment
};

// ============================================================
// Program Header Flags (p_flags)
// ============================================================
enum class SegmentFlag : uint32_t {
    None    = 0,
    X       = 1,  // Execute
    W       = 2,  // Write
    R       = 4,  // Read
    RW      = 6,
    RX      = 5,
    RWX     = 7
};

inline SegmentFlag operator|(SegmentFlag a, SegmentFlag b) noexcept {
    return static_cast<SegmentFlag>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool hasFlag(SegmentFlag flags, SegmentFlag flag) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// ============================================================
// Section Header Types (sh_type)
// ============================================================
enum class SectionType : uint32_t {
    Null          = 0,
    ProgBits      = 1,
    SymTab        = 2,
    StrTab        = 3,
    Rela          = 4,
    Hash          = 5,
    Dynamic       = 6,
    Note          = 7,
    NoBits        = 8,
    Rel           = 9,
    ShLib         = 10,
    DynSym        = 11,
    InitArray     = 14,
    FiniArray     = 15,
    PreinitArray  = 16,
    Group         = 17,
    SymTabShndx   = 18,
    Num           = 19,
    LoOS          = 0x60000000,
    HiOS          = 0x6FFFFFFF,
    LoProc        = 0x70000000,
    HiProc        = 0x7FFFFFFF,

    // GNU-specific
    GnuAttributes = 0x6FFFFFF5,
    GnuHash       = 0x6FFFFFF6,
    GnuLibList    = 0x6FFFFFF7,
    GnuVerDef     = 0x6FFFFFFD,
    GnuVerNeed    = 0x6FFFFFFE,
    GnuVerSym     = 0x6FFFFFFF,
};

// ============================================================
// Section Header Flags (sh_flags)
// ============================================================
enum class SectionFlag : uint64_t {
    None       = 0,
    Write      = 0x1,
    Alloc      = 0x2,
    ExecInstr  = 0x4,
    Merge      = 0x10,
    Strings    = 0x20,
    InfoLink   = 0x40,
    LinkOrder  = 0x80,
    OsNonConforming = 0x100,
    Group      = 0x200,
    TLS        = 0x400,
    Compressed = 0x800,
    Ordered    = 0x4000000,
    Exclude    = 0x8000000
};

// ============================================================
// Segment (PT_LOAD) flags used by Qualcomm Firehose
// ============================================================
inline constexpr uint32_t ELF_PT_LOAD = 1;

// ============================================================
// Known section name constants (for quick lookup)
// ============================================================
inline constexpr const char* SHN_UNDEF_STR     = "";
inline constexpr const char* SHN_TEXT_STR      = ".text";
inline constexpr const char* SHN_DATA_STR      = ".data";
inline constexpr const char* SHN_BSS_STR       = ".bss";
inline constexpr const char* SHN_RODATA_STR    = ".rodata";

// ============================================================
// Helper: convert machine enum to readable string
// ============================================================
inline const char* machineToString(ElfMachine m) noexcept {
    switch (m) {
        case ElfMachine::None:       return "None";
        case ElfMachine::SPARC:      return "SPARC";
        case ElfMachine::x86:        return "x86";
        case ElfMachine::M68K:       return "M68K";
        case ElfMachine::MIPS:       return "MIPS";
        case ElfMachine::PPC:        return "PowerPC";
        case ElfMachine::PPC64:      return "PowerPC64";
        case ElfMachine::ARM:        return "ARM";
        case ElfMachine::SH:         return "SuperH";
        case ElfMachine::IA_64:      return "IA-64";
        case ElfMachine::x86_64:     return "x86-64";
        case ElfMachine::AARCH64:    return "AArch64";
        case ElfMachine::RISC_V:     return "RISC-V";
        case ElfMachine::BPF:        return "BPF";
        default:                     return "Unknown";
    }
}

// ============================================================
// Helper: segment type to readable string
// ============================================================
inline const char* segmentTypeToString(SegmentType t) noexcept {
    switch (t) {
        case SegmentType::Null:    return "PT_NULL";
        case SegmentType::Load:    return "PT_LOAD";
        case SegmentType::Dynamic: return "PT_DYNAMIC";
        case SegmentType::Interp:  return "PT_INTERP";
        case SegmentType::Note:    return "PT_NOTE";
        case SegmentType::ShLib:   return "PT_SHLIB";
        case SegmentType::Phdr:    return "PT_PHDR";
        case SegmentType::TLS:     return "PT_TLS";
        case SegmentType::QDSS6:   return "PT_QDSS6";
        default:                   return "PT_UNKNOWN";
    }
}

} // namespace elf
} // namespace mbootcore
