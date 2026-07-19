# Protocol Specifications

## Overview

MBootCore supports two Qualcomm protocols — Sahara and Firehose — which operate sequentially in the boot chain. Sahara handles initial device discovery and programmer upload. Firehose handles all flash operations after the programmer is loaded.

## Protocol Comparison

| Aspect | Sahara | Firehose |
|--------|--------|----------|
| Message Format | Binary (8-byte header: command + length) | XML text (null-terminated strings) |
| Direction | Target-driven (device sends first) | Host-driven (host sends first) |
| Purpose | Upload boot programmer | Program, read, erase flash |
| State Machine | Complex (command/response pairs) | Simple (4 states: Idle, Configured, Busy, Error) |
| Streaming | Simple image transfer (chunked READ_DATA) | Chunked with progress callbacks |
| Versioning | V2 (legacy) / V3 (extended chip info) | Single protocol (XML schema) |
| Commands | 19 packet types | 13 commands |
| Error Handling | 41 NAK codes | ACK/NAK XML responses |

## Transport Model

Both protocols operate over the same transport medium — typically USB bulk endpoints.

```
USB Bulk OUT (Host → Device): command packets
USB Bulk IN  (Device → Host): response packets
```

The transport layer (`ITransport`) abstracts the underlying connection:

- `USBTransport` — WinUSB (Windows) / LibUSB (Linux/macOS)
- `SerialTransport` — POSIX termios / Win32 API
- `TCPTransport` — POSIX sockets / WinSock2

## Sahara Protocol

Sahara is a binary, target-driven protocol used by Qualcomm SoCs in Emergency Download (EDL) mode. The device initiates all image transfer commands; the host responds.

**Key properties:**
- All multi-byte fields are Little-Endian
- Every packet has an 8-byte header: `[command:u32][length:u32]`
- The protocol is command/response — each command expects a specific response
- V3 is backward-compatible at the handshake level; adds one new command ID (`READ_CHIPID_V3 = 0x0A`) and extended chip-ID fields

**Supported commands:**
See the [Sahara Architecture Document](../architecture/Sahara.md) for the complete list of 19 packet types.

## Firehose Protocol

Firehose is an XML-based, host-driven protocol used by Qualcomm SoCs after Sahara completes programmer loading. The host sends XML commands and the device responds with XML responses.

**Key properties:**
- All communication is text-based XML over a raw transport
- Every command expects a response — strictly command/response
- Commands are self-describing XML strings
- The protocol supports streaming for large data transfers (program, read)
- Firehose begins with a `<configure>` command

**Supported commands:**
See the [Firehose Architecture Document](../architecture/Firehose.md) for the complete list of 13 XML commands.

## Boot Chain

```
Power-on → PBL detects EDL mode → Sahara negotiates → Programmer loaded
→ Programmer executes → Firehose takes over → Flash operations complete → Reset
```

## Protocol Negotiation

MBootCore uses a scoring-based negotiation system:

1. `IDeviceDetector` enumerates and identifies devices via USB VID/PID, transport probing, or other vendor-specific methods
2. `IProtocolNegotiator` scores each detected device against protocol capabilities (confidence score 0–200+)
3. `ProtocolNegotiationEngine` selects the best match
4. `IProtocolFactory` creates the appropriate protocol adapter and pipeline

Scoring uses descending priority:
- PK Hash match: 80 points
- MSM ID match: 50 points
- Vendor match: 40 points
- Chipset match: 30 points
- Protocol match: 15 points
- Storage type match: 10 points
