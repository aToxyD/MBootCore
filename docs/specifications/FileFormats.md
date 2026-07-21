# File Format Specifications

## ELF Format

MBootCore uses the ELF (Executable and Linkable Format) for boot programmer binaries, specifically Firehose programmer ELF files.

### Supported Variants

| Variant | Header Size | Pointer Width |
|---------|-------------|---------------|
| ELF32 | 52 bytes | 4 bytes |
| ELF64 | 64 bytes | 8 bytes |

### ELF Header

**32-bit header layout:**

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | e_ident[0..3] | Magic: `\x7FELF` |
| 0x04 | 1 | ei_class | 1=32-bit, 2=64-bit |
| 0x05 | 1 | ei_data | 1=LE, 2=BE |
| 0x06 | 1 | ei_version | Must be 1 |
| 0x07 | 1 | ei_osabi | OS/ABI identification |
| 0x08 | 1 | ei_abiversion | ABI version |
| 0x09 | 7 | ei_pad | Padding |
| 0x10 | 2 | e_type | ET_NONE(0), ET_REL(1), ET_EXEC(2), ET_DYN(3) |
| 0x12 | 2 | e_machine | Architecture |
| 0x14 | 4 | e_version | Must be 1 |
| 0x18 | 4 | e_entry | Virtual address of entry point |
| 0x1C | 4 | e_phoff | Program header table offset |
| 0x20 | 4 | e_shoff | Section header table offset |
| 0x24 | 4 | e_flags | Processor-specific flags |
| 0x28 | 2 | e_ehsize | ELF header size |
| 0x2A | 2 | e_phentsize | Program header entry size |
| 0x2C | 2 | e_phnum | Number of program headers |
| 0x2E | 2 | e_shentsize | Section header entry size |
| 0x30 | 2 | e_shnum | Number of section headers |
| 0x32 | 2 | e_shstrndx | Section header string table index |

### Supported Machines

| Machine | Value |
|---------|-------|
| EM_ARM | 40 |
| EM_AARCH64 | 183 |
| EM_RISCV | 243 |

### Program Header Types

| Type | Value | Description |
|------|-------|-------------|
| PT_NULL | 0 | Unused |
| PT_LOAD | 1 | Loadable segment |
| PT_DYNAMIC | 2 | Dynamic linking |
| PT_INTERP | 3 | Interpreter path |
| PT_NOTE | 4 | Note section |
| PT_SHLIB | 5 | Shared library |
| PT_PHDR | 6 | Program header table |
| PT_TLS | 7 | Thread-local storage |
| PT_QDSS6 | 0x70000001 | Qualcomm-specific |

### ELF Processing

The ELF Engine:
1. Parses the binary ELF structure into strongly-typed models
2. Validates magic, header fields, segment alignment, and overlap
3. Builds a memory image by merging PT_LOAD segments sorted by virtual address
4. Preserves the entry point for programmer execution

---

## GPT Format

GPT (GUID Partition Table) is the modern standard for disk partitioning, part of the UEFI specification.

### GPT vs MBR

| Feature | MBR | GPT |
|---------|-----|-----|
| Max partitions | 4 primary | 128 (default) |
| Max disk size | 2 TiB | 9.4 ZiB |
| Redundancy | None | Primary + Backup |
| CRC protection | No | Header + Entry CRC32 |
| Partition naming | No | UTF-16LE names (36 chars) |
| Unique IDs | No | GUID for disk + each partition |

### Disk Layout (512-byte sectors)

```
LBA 0:    Protective MBR (512 bytes)
LBA 1:    Primary GPT Header (512 bytes)
LBA 2–33: Primary Partition Entries (128 × 128 bytes)
LBA 34:   First Usable LBA
  ...
LBA −34:  Last Usable LBA
LBA −33–2: Backup Partition Entries
LBA −1:   Backup GPT Header (512 bytes)
```

### Protective MBR (LBA 0)

| Offset | Size | Field | Value |
|--------|------|-------|-------|
| 0x000 | 446B | Bootstrap code | Zeros |
| 0x1BE | 16B | Partition Entry 1 | Single partition covering entire disk |
| 0x1FE | 2B | Signature | 0xAA55 |

The protective MBR partition entry:
- Status: 0x00 (inactive)
- Type: 0xEE (GPT protective)
- Start LBA: 1
- Size: disk size in sectors (or 0xFFFFFFFF for >2TiB)

### Primary GPT Header (LBA 1)

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 8 | Signature: "EFI PART" |
| 0x08 | 4 | Revision (0x00010000) |
| 0x0C | 4 | Header size (92) |
| 0x10 | 4 | Header CRC32 |
| 0x14 | 4 | Reserved (0) |
| 0x18 | 8 | Current LBA (1) |
| 0x20 | 8 | Backup LBA |
| 0x28 | 8 | First usable LBA (34) |
| 0x30 | 8 | Last usable LBA |
| 0x38 | 16 | Disk GUID |
| 0x48 | 8 | Partition entry start LBA (2) |
| 0x50 | 4 | Number of partition entries |
| 0x54 | 4 | Partition entry size (128) |
| 0x58 | 4 | Partition entries CRC32 |

### Partition Entry (128 bytes each)

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 16 | Partition type GUID |
| 0x10 | 16 | Unique partition GUID |
| 0x20 | 8 | Starting LBA |
| 0x28 | 8 | Ending LBA (inclusive) |
| 0x30 | 8 | Attributes |
| 0x38 | 72 | Partition name (UTF-16LE, 36 chars) |

### CRC32 Computation

- Header CRC32: computed over the header with CRC32 field set to 0, from offset 0 to header size
- Entry CRC32: computed over the entire partition entry array
- Uses zlib `crc32()` function

### Well-Known Partition Type GUIDs

| Name | GUID |
|------|------|
| EFI System Partition | C12A7328-F81F-11D2-BA4B-00A0C93EC93B |
| Basic data partition | EBD0A0A2-B9E5-4433-87C0-68B6B72699C7 |
| Microsoft reserved | E3C9E316-0B5C-4DB8-817D-F92DF00215AE |
| Linux filesystem | 0FC63DAF-8483-4772-8E79-3D69D8477DE4 |
| Linux swap | 0657FD6D-A4AB-43C4-84E5-0933C84B4F4F |
| Linux LVM | E6D6D379-F507-44C2-A23C-238F2A3DF928 |
| Android boot | 49A4D17F-93A3-45C1-A0DE-F50B2EBE2599 |
| Android system | 3C792A29-2161-46D9-8CE6-57D3E0F2F5A0 |
| Qualcomm UFS | 13AC7B90-F38F-11DF-8C7E-0002A5D5C51B |

### GPT Validation

MBootCore's GPTParser validates:
1. Protective MBR at LBA 0 (signature 0xAA55, type 0xEE)
2. GPT header signature ("EFI PART"), revision, header size
3. Header CRC32 matches computed value
4. Partition entry CRC32 matches computed value
5. LBA ranges within disk bounds
6. No overlapping partitions
7. Primary and backup GPT consistency
