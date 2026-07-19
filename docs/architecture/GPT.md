# GPT Engine

## Overview

The GPT Engine provides a complete, protocol-independent GPT (GUID Partition Table) implementation for reading, writing, managing, and recovering GPT partition tables. It is designed as a Clean Architecture layer with zero protocol dependencies — all I/O goes through `IFlashDevice`.

### Design Goals

- **Protocol-independent**: No knowledge of Sahara, Firehose, Qualcomm, or any SoC
- **RAII + Value semantics**: Partition entries are value types, managers own their state
- **Recovery-first**: Primary ↔ Backup redundancy is built into every operation
- **Deterministic CRC**: All CRC32 computations use zlib `crc32()`
- **UTF-16LE natively**: Partition names are stored as UTF-16LE in GPT; converted to/from `std::string` on access

## Layer Architecture

```
Application (Session)
    │
    ▼
PartitionManager    ← Public API for all partition operations
    │
    ├── GPTParser   ← Parses binary GPT data from IFlashDevice
    └── GPTWriter   ← Serializes GPT structures to IFlashDevice
            │
            ▼
        IFlashDevice
            │
            ▼
    Transport Layer (USB / Serial / TCP)
```

### Dependency Flow

- `PartitionManager` — depends on `GPTParser`, `GPTWriter`, and `IFlashDevice`
- `GPTParser` — depends on `IFlashDevice` for raw reads
- `GPTWriter` — depends on `IFlashDevice` for raw writes
- All types use `Guid`, `GPTHeader`, `PartitionEntry`, `GPTLayout`, `PartitionInfo`, `PartitionRange` from `GPTModels.hpp`

## File Layout

```
lib/include/mbootcore/gpt/
├── Guid.hpp             ← Guid type (16 bytes), fromString, toString, hash
├── GPTModels.hpp        ← GPTHeader, PartitionEntry, GPTLayout, PartitionInfo,
│                            PartitionRange, PartitionTypes (well-known GUIDs),
│                            PartitionAttribute, GPTTable, ProtectiveMBREntry
├── GPTParser.hpp        ← GPTParser interface (parse, parsePrimary, parseBackup)
├── GPTWriter.hpp        ← GPTWriter interface (writePrimary, writeBackup, writeEntries)
├── PartitionManager.hpp ← PartitionManager (open, list, find, read/write/erase, backup/restore)
├── IImageReader.hpp     ← Image reader interface
└── ImageReaders.hpp     ← FileImageReader, BufferImageReader

lib/src/gpt/
├── GPTParser.cpp        ← GPTParser implementation
├── GPTWriter.cpp        ← GPTWriter implementation
├── PartitionManager.cpp ← PartitionManager implementation
└── ImageReaders.cpp     ← Image reader implementations
```

## Components

### Guid

A 16-byte GUID type with string conversion:

```cpp
struct Guid {
    uint8_t data[16];       // Big-endian storage (network order)

    static Guid fromString(std::string_view str);  // "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
    std::string toString() const;
    bool isValid() const;
    bool isNull() const;
};
```

### GPTModels

Core model types:

- **GPTHeader**: signature ("EFI PART"), revision, header size, CRC32, LBAs, disk GUID, partition entry information
- **PartitionEntry**: type GUID, unique GUID, start/end LBA, attributes, name (UTF-16LE)
- **GPTLayout**: primary header, backup header, partition entries, disk geometry
- **PartitionInfo**: name, type, unique GUID, start/end LBA, size, attributes
- **PartitionRange**: start LBA, end LBA, partition reference
- **PartitionTypes**: inline constexpr `Guid` constants for well-known partition types
- **PartitionAttribute**: bit-field flags (required, no block IO, legacy BIOS bootable, etc.)
- **GPTTable**: parsed table holding all entries plus validation status
- **ProtectiveMBREntry**: MBR partition entry with type 0xEE

### GPTParser

Reads and validates GPT structures from an `IFlashDevice`:

- `parse(device)` — parses both primary and backup GPT, validates consistency
- `parsePrimary(device)` — parses primary GPT only
- `parseBackup(device)` — parses backup GPT only

Validation:
1. Protective MBR at LBA 0 (signature 0xAA55, partition type 0xEE)
2. GPT header signature, revision (0x00010000), header size (92)
3. Header CRC32 matches computed value
4. Partition entry count and entry size are within documented bounds
5. Arithmetic overflow checks on all entry-count × entry-size calculations
6. Partition entry CRC32 matches computed value
7. LBA ranges within disk bounds
8. No overlapping partitions
9. Primary and backup GPT consistency (CRC mismatch triggers recovery)

#### Defensive Parsing (Hardening)

MBootCore validates GPT header fields **before** any memory allocation,
buffer reservation, or sector read derived from those values:

- **Entry count bounds**: `numberOfPartitionEntries` must be > 0 and ≤ 4096.
  The GPT specification commonly uses 128 entries. 4096 exceeds typical
  real-world layouts while preventing denial-of-service via excessive
  allocation requests from a crafted header.

- **Entry size bounds**: `sizeOfPartitionEntry` must be ≥ 128 and ≤ 512 bytes.
  The GPT specification defines 128-byte entries. Values outside this range
  indicate corruption or a non-standard layout.

- **Overflow-safe arithmetic**: Every multiplication of entry count × entry
  size is checked for uint64 overflow before use. Sector count calculations
  use overflow-safe helpers.

- **Allocation policy**: The parser owns allocation policy. A device can never
  force unbounded memory allocation through crafted GPT metadata. Malformed
  headers are rejected with `GPTInvalidHeader` before any `reserve()`,
  `resize()`, `readSectors()`, or CRC computation occurs.

### GPTWriter

Creates and writes GPT structures:

- `writePrimary(device, layout)` — writes primary GPT header + entries at LBA 1+
- `writeBackup(device, layout)` — writes backup GPT header + entries at end of disk
- `writeEntries(device, layout)` — writes partition entries only

The writer:
1. Computes header CRC32 (with CRC field zeroed)
2. Computes partition entries CRC32 over the full array
3. Builds protective MBR with type 0xEE
4. Writes in correct LBA order

### PartitionManager

High-level API for all partition operations:

```cpp
class PartitionManager {
    Result<Unit> open(IDevice& device);
    Result<Unit> close();

    Result<std::vector<PartitionInfo>> listPartitions();
    Result<PartitionInfo> findPartition(std::string_view name);
    Result<PartitionInfo> findPartition(Guid typeGuid);

    Result<ByteBuffer> readPartition(std::string_view name);
    Result<Unit> writePartition(std::string_view name, const ByteBuffer& data);
    Result<Unit> erasePartition(std::string_view name);

    Result<Unit> backupGPT(const std::string& backupPath);
    Result<Unit> restoreGPT(const std::string& backupPath);

    Result<Unit> recover();
};
```

Recovery:
- If primary GPT is corrupt and backup is valid → restore primary from backup
- If backup GPT is corrupt and primary is valid → restore backup from primary
- If both are corrupt → returns error (manual intervention required)

### ImageReaders

Interface for reading raw image data:

- `IFileImageReader` — reads from filesystem
- `IBufferImageReader` — reads from memory buffer
- Both implement `IImageReader` interface with `read()` and `size()` methods

## Disk Layout (512-byte sectors)

```
LBA 0:    Protective MBR (512 bytes)
LBA 1:    Primary GPT Header (512 bytes)
LBA 2–33: Primary Partition Entries (128 × 128 bytes = 32 sectors)
LBA 34:   First Usable LBA
  ...
LBA −34:  Last Usable LBA
LBA −33–2: Backup Partition Entries (32 sectors)
LBA −1:   Backup GPT Header (512 bytes)
```
