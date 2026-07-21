# Firehose Protocol

## Overview

Firehose is an XML-based, host-driven protocol used by Qualcomm SoCs after Sahara completes programmer loading. The host sends XML commands and the device responds with XML responses.

**Key Properties:**
- All communication is text-based XML over a raw transport (USB bulk, serial)
- Every command expects a response — Firehose is strictly command/response
- Commands are self-describing XML strings, not fixed binary structures
- The protocol supports streaming for large data transfers (program, read)
- Firehose begins with a `<configure>` command

## Position in Boot Chain

```
Power-on → PBL → Sahara (EDL) → Programmer Loaded → Firehose
```

1. **PBL** detects EDL mode (USB descriptor 0x9008 / 0x05C6)
2. **Sahara** runs first: negotiates version, uploads the programmer
3. Once Sahara completes (DONE_RESP → RESET), the programmer executes and exposes a Firehose XML interface over the same transport
4. **Firehose** takes over: configure, program, read, erase, etc.

## Architecture

```
Session
  │
  ▼
IFlashDevice  ◄── Generic Layer (no Firehose knowledge)
  │
FirehoseAdapter  ◄── Bridge
  │
FirehoseProtocol  ◄── Protocol implementation
  ├── FirehoseXmlEngine     — XML serialization / parsing
  ├── FirehoseStreamEngine  — Chunked data streaming
  ├── ChunkEngine           — Reusable chunk transfer
  ├── FlashContext          — Storage configuration context
  └── FirehosePackets       — 13 typed command/response classes
```

### Layer Isolation

- **Generic Layer** (`IFlashDevice`) knows nothing about Firehose
- **FirehoseAdapter** is the sole bridge implementing `IFlashDevice` using `FirehoseProtocol`
- **FirehoseProtocol** has no dependency on Generic Layer headers
- **ChunkEngine** is a standalone utility with zero Firehose dependencies

## State Machine

Firehose uses a 4-state machine:

| State | Value | Description |
|-------|-------|-------------|
| Idle | 0 | Initial state, waiting for configure |
| Configured | 1 | Ready for flash operations |
| Busy | 2 | Operation in progress |
| Error | 3 | Protocol or operation error |

Transitions:
- Idle → Configured: successful `<configure>` response (ACK)
- Configured → Busy: operation started (program, read, erase, etc.)
- Busy → Configured: operation complete
- Any → Error: NAK or transport failure

## Command Reference

### Transport Model

- **Host → Device**: Write null-terminated XML command string, optionally followed by raw binary chunks
- **Device → Host**: Read null-terminated XML response string, optionally followed by raw binary chunks
- All XML responses use the command name as root element with `value="ACK"` or `value="NAK"`

### ACK/NACK Format

```xml
<!-- Success -->
<command value="ACK"/>

<!-- Error -->
<command value="NAK" description="Human-readable error message"/>
```

### Command Details

#### `<configure>`

Initialize Firehose protocol with memory configuration. Must be sent before most other commands.

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `MemoryName` | string | "ufs" | Target memory type (ufs, emmc, nand, nor, spi) |
| `ZLPAwareHost` | uint | 1 | Zero-length packet awareness |
| `SkipWrite` | uint | 0 | Skip write operations (diagnostic) |
| `MaxPayloadSizeToTarget` | uint | 1048576 | Max bytes per host→device chunk |
| `MaxPayloadSizeFromTarget` | uint | 1048576 | Max bytes per device→host chunk |
| `Mode` | uint | 0 | Reserved for future use |

```xml
<configure MemoryName="ufs" ZLPAwareHost="1" SkipWrite="0"
           MaxPayloadSizeToTarget="1048576"
           MaxPayloadSizeFromTarget="1048576" Mode="0"/>
```

Response: ACK with echoed attributes + `value="ACK"`, or NAK with description.

#### `<program>`

Write data to flash storage.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Sector size in bytes |
| `num_partition_sectors` | uint | Number of sectors to write |
| `start_sector` | uint | Starting sector address |
| `physical_partition_number` | uint | Physical partition number |
| `filename` | string | Partition filename |

The program command is followed by raw binary data chunks via the streaming engine.

#### `<read>`

Read data from flash storage.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Sector size in bytes |
| `num_partition_sectors` | uint | Number of sectors to read |
| `start_sector` | uint | Starting sector address |
| `physical_partition_number` | uint | Physical partition number |
| `filename` | string | Partition filename |

Response includes raw binary data chunks when `raw_mode="true"`.

#### `<erase>`

Erase flash sectors.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Sector size in bytes |
| `num_partition_sectors` | uint | Number of sectors to erase |
| `start_sector` | uint | Starting sector address |
| `physical_partition_number` | uint | Physical partition number |
| `filename` | string | Partition filename |
| `label` | string | Partition label |

#### `<peek>`

Read memory at a physical address.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Memory access alignment |
| `num_partition_sectors` | uint | Number of units to read |
| `start_sector` | uint | Physical address |

#### `<poke>`

Write memory at a physical address.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Memory access alignment |
| `num_partition_sectors` | uint | Number of units to write |
| `start_sector` | uint | Physical address |
| `data` | hex | Hex-encoded data to write |

#### `<patch>`

Apply memory patches.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Alignment |
| `num_partition_sectors` | uint | Number of patches |
| `start_sector` | uint | Base address |

#### `<reset>`

Reset the target device.

#### `<power>`

Power management operations.

| Attribute | Type | Description |
|-----------|------|-------------|
| `value` | string | "on" or "off" |

#### `<NOP>`

No operation — used for keepalive or latency testing.

#### `<getstorageinfo>`

Retrieve storage configuration information.

Response contains `memb_type`, `page_size`, `block_size`, `total_blocks`, and other storage parameters.

#### `<getsha256digest>`

Request SHA-256 digest of a storage region.

| Attribute | Type | Description |
|-----------|------|-------------|
| `SECTOR_SIZE` | uint | Sector size |
| `num_partition_sectors` | uint | Number of sectors |
| `start_sector` | uint | Starting sector |

#### `<configurememory>`

Additional memory configuration for complex setups.

## Command Summary

| Command | Read | Write | Erase | Description |
|---------|------|-------|-------|-------------|
| configure | — | — | — | Memory initialization |
| program | — | ✓ | — | Write data to flash |
| read | ✓ | — | — | Read data from flash |
| erase | — | — | ✓ | Erase flash sectors |
| peek | ✓ | — | — | Read RAM/registers |
| poke | — | ✓ | — | Write RAM/registers |
| patch | — | ✓ | — | Patch memory regions |
| reset | — | — | — | Device reset |
| power | — | — | — | Power management |
| NOP | — | — | — | No operation |
| getstorageinfo | — | — | — | Storage configuration |
| getsha256digest | — | — | — | SHA-256 digest |
| configurememory | — | — | — | Memory configuration |

## Streaming

Firehose supports chunked streaming for large data transfers:

- **Program streaming**: host→device, raw binary data chunks after XML command
- **Read streaming**: device→host, raw binary data chunks after XML response

The `ChunkEngine` handles:
- Configurable chunk sizes (up to `MaxPayloadSizeToTarget` / `MaxPayloadSizeFromTarget`)
- Progress reporting via callback
- Cancellation via atomic flag
- Partial transfer recovery

## Error Handling

Firehose errors are returned as NAK XML responses with human-readable descriptions:

```xml
<program value="NAK" description="SECTOR_SIZE_INCR_SUPPORT not supported"/>
```

MBootCore maps NAK responses to typed error codes (10 Firehose-specific error codes in the 0x0700 range).

## Implementation Components

| Component | File | Responsibility |
|-----------|------|----------------|
| FirehoseProtocol | `core/protocols/firehose/` | Full protocol implementation |
| FirehoseXmlEngine | `core/protocols/firehose/` | XML serialization/parsing with entity escaping |
| FirehosePackets | `core/protocols/firehose/` | 13 typed command classes |
| FirehoseStreamEngine | `core/protocols/firehose/` | Chunked data streaming |
| ChunkEngine | `core/protocols/firehose/` | Reusable chunk transfer |
| FlashContext | `core/protocols/firehose/` | Storage configuration context |
| FirehoseAdapter | `generic/adapter/` | Bridge to IFlashDevice |

## See Also

- [Sahara Protocol](Sahara.md) — companion protocol for Qualcomm EDL mode
- [Generic Flash](GenericFlash.md) — protocol-agnostic flash operations
- [Overview](Overview.md) — architecture layer diagram
