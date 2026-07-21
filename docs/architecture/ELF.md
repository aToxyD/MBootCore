# ELF Engine

## Overview

The ELF Engine is a standalone, protocol-agnostic component for parsing, validating, and loading ELF (Executable and Linkable Format) binaries — specifically the Firehose programmer ELF files used in Qualcomm flashing workflows.

The ELF Engine has zero knowledge of Sahara, Firehose, or any protocol. It is a pure ELF processing subsystem with no external dependencies beyond the standard library (C++17 STL containers).

## Architecture

```
ELF Engine
├── ElfTypes.hpp             — Constants, enums, string helpers
├── ElfModels.hpp            — Strongly typed data models
├── ElfParser.hpp/cpp        — Binary ELF parser
├── ElfValidator.hpp/cpp     — Validation rules engine
├── MemoryImageBuilder.hpp/cpp  — Memory image construction
├── IProgrammerExecutor.hpp     — Execution interface
└── VirtualProgrammer.hpp/cpp   — Simulated executor for testing
```

### Layer Isolation

No component in the ELF Engine includes any protocol header (Sahara, Firehose), any transport header, any loader framework header, or any generic layer header.

## Components

### ElfTypes

Defines ELF constants used throughout the engine:

- **Magic**: `ELFMAG = {0x7F, 'E', 'L', 'F'}`
- **Class**: `ELFCLASS32(1)`, `ELFCLASS64(2)`
- **Encoding**: `ELFDATA2LSB(1)`, `ELFDATA2MSB(2)`
- **Machine**: `EM_ARM(40)`, `EM_AARCH64(183)`, `EM_RISCV(243)`
- **Segment type**: `PT_NULL(0)`, `PT_LOAD(1)`, `PT_DYNAMIC(2)`, `PT_INTERP(3)`, `PT_NOTE(4)`, `PT_SHLIB(5)`, `PT_PHDR(6)`, `PT_TLS(7)`, `PT_QDSS6(0x70000001)`
- **Section type**: `SHT_NULL(0)`, `SHT_PROGBITS(1)`, `SHT_SYMTAB(2)`, `SHT_STRTAB(3)`, `SHT_RELA(4)`, `SHT_HASH(5)`, `SHT_DYNAMIC(6)`, `SHT_NOTE(7)`, `SHT_NOBITS(8)`, `SHT_REL(9)`, `SHT_DYNSYM(11)`
- **Segment flags**: `PF_X(1)`, `PF_W(2)`, `PF_R(4)`

### ElfModels

Strongly typed models representing parsed ELF structures:

```cpp
struct ElfFile {
    std::vector<uint8_t> rawData;
    ElfHeader header;
    std::vector<ProgramHeader> programHeaders;
    std::vector<SectionHeader> sectionHeaders;
};

struct ElfHeader {
    uint8_t ident[16];          // Magic, class, encoding, version, OS/ABI
    uint16_t type;              // ET_EXEC, ET_DYN, etc.
    uint16_t machine;           // EM_ARM, EM_AARCH64, etc.
    uint32_t version;
    uint64_t entry;             // Entry point virtual address
    uint64_t phoff;             // Program header table offset
    uint64_t shoff;             // Section header table offset
    uint32_t flags;
    uint16_t ehsize;            // ELF header size
    uint16_t phentsize;         // Program header entry size
    uint16_t phnum;             // Number of program headers
    uint16_t shentsize;         // Section header entry size
    uint16_t shnum;             // Number of section headers
    uint16_t shstrndx;          // Section header string table index
};
```

Helper methods:
- `is32Bit()` / `is64Bit()` — class identification
- `isLittleEndian()` / `isBigEndian()` — encoding detection
- `entryPoint()` — entry point address
- `isLoadable()` — checks if segment type is PT_LOAD
- `segmentsOverlap()` — overlap detection between segments

### ElfParser

Parses raw ELF binary data into strongly-typed models:

- Supports 32-bit and 64-bit ELF
- Validates magic bytes on entry
- Handles both endianness (with primary LE support)
- Extracts program headers for PT_LOAD segments
- Extracts section headers for symbol/string tables

### ElfValidator

Applies validation rules to parsed ELF structures:

| Rule | Description |
|------|-------------|
| Magic | Verifies `\x7FELF` signature |
| Class | Must be 32-bit or 64-bit |
| Encoding | Must be LE or BE |
| Header size | Must match expected for class |
| Program headers | Must not overlap in virtual address space |
| Segment alignment | Reports alignment warnings |

### MemoryImageBuilder

Constructs a contiguous memory image from PT_LOAD segments:

1. Sorts segments by virtual address (ascending)
2. Merges overlapping or adjacent segments
3. Preserves the entry point
4. Returns a `MemoryImage` with `data`, `baseAddress`, `size`, and `entryPoint`

### IProgrammerExecutor

5-stage execution interface for boot programmers:

| Stage | Description |
|-------|-------------|
| `load()` | Load the programmer binary |
| `verify()` | Verify the loaded programmer |
| `prepare()` | Prepare device for transfer |
| `transfer()` | Transfer programmer to device |
| `start()` | Execute the programmer |

### VirtualProgrammer

Simulated execution for testing without hardware:

- Supports all 5 execution stages
- Configurable success/failure per stage
- State machine tracking (Idle → Loaded → Verified → Prepared → Transferred → Started → Error)
- Progress callback support
- Timeout and cancellation simulation

## Error Codes

| Code | Value | Description |
|------|-------|-------------|
| InvalidElf | 0x0404 | ELF malformed or invalid |
| ElfSegmentOverlap | 0x0600 | PT_LOAD segments overlap in vaddr |
| ElfTruncated | 0x0601 | File too short |
| ElfUnsupportedMachine | 0x0602 | Machine type unsupported |

## Integration

The ELF Engine is used by:
- **LoaderFramework::ElfInspector** — header inspection for loader matching
- **FirehoseAdapter** — parse programmer ELF for upload
- **Boot Pipeline** — Stage 3 (ELF Parsing) and Stage 4 (Programmer Upload)

## See Also

- [Firehose Protocol](Firehose.md) — protocol that uses ELF programmer files
- [Loader Framework](LoaderFramework.md) — loader matching and ELF inspection
- [Overview](Overview.md) — architecture layer diagram
