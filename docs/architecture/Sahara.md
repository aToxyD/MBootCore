# Sahara Protocol

## Overview

Sahara is a binary, target-driven protocol used by Qualcomm SoCs in Emergency Download (EDL) mode. The device (target) initiates all image transfer commands; the host responds.

**Key Properties:**
- All multi-byte fields are Little-Endian
- Every packet has an 8-byte header: `[command:u32][length:u32]`
- The protocol is command/response â€” each command expects a specific response
- Two protocol versions: V2 and V3 (extended chip info)
- V3 is backward-compatible at the handshake level

## Packet Format

### Common Header (8 bytes)

```
Offset  Size  Field     Description
------  ----  -----     -----------
0       4     command   Command ID (uint32 LE)
4       4     length    Total packet length including header (uint32 LE)
```

### Command Summary

| ID | Name | Direction | Description |
|----|------|-----------|-------------|
| 0x01 | HELLO_REQ | Device â†’ Host | Protocol version announcement |
| 0x02 | HELLO_RESP | Host â†’ Device | Version acknowledgment |
| 0x03 | READ_DATA | Host â†’ Device | Image data transfer |
| 0x04 | END_IMAGE | Device â†’ Host | Image transfer complete |
| 0x05 | DONE_REQ | Host â†’ Device | Operation complete |
| 0x06 | DONE_RESP | Device â†’ Host | Operation acknowledgment |
| 0x07 | RESET | Host â†’ Device | Device reset |
| 0x08 | MEMORY_DEBUG | Bidirectional | Memory debug operations |
| 0x09 | READ_CHIPID_V2 | Device â†’ Host | V2 chip identification |
| 0x0A | READ_CHIPID_V3 | Device â†’ Host | V3 extended chip identification |
| 0x0B | CMD_SWITCH_MODE | Host â†’ Device | Mode switching |
| 0x0C | SEND_COMMAND_DATA | Host â†’ Device | Command data transfer |
| 0x0D | COMMAND_DATA_RESP | Device â†’ Host | Command data response |
| 0x0E | UNSUPPORTED_CMD_RESP | Device â†’ Host | Unsupported command |
| 0x0F | EXECUTE_REQ | Host â†’ Device | Execute command |
| 0x10 | EXECUTE_RESP | Device â†’ Host | Execute result |
| 0x11 | EXECUTE_DATA | Bidirectional | Execute data transfer |
| 0x12 | EXECUTE_DATA_RESP | Device â†’ Host | Execute data response |
| 0x13 | NAK | Device â†’ Host | Negative acknowledgment (41 error codes) |
| 0x14 | ESD | Device â†’ Host | Early shutdown detection |

## V2 vs V3 Handshake

### V2 Flow

```
Device sends  HELLO_REQ (command=0x01, version)
Host sends    HELLO_RESP (command=0x02, version)
Device sends  READ_CHIPID_V2 (command=0x09, chip info)
```

### V3 Flow

```
Device sends  HELLO_REQ (command=0x01, version)
Host sends    HELLO_RESP (command=0x02, version)
Device sends  READ_CHIPID_V3 (command=0x0A, extended chip info)
```

## Image Transfer

Sahara transfers images (boot programmers) to the device using chunked READ_DATA commands:

```
Host sends    READ_DATA (command=0x03, offset, length)
Device sends  HELLO_REQ (command=0x01, version with updated state)
  ... repeats for each chunk ...
Host sends    DONE_REQ (command=0x05)
Device sends  DONE_RESP (command=0x06, status)
Host sends    RESET (command=0x07)
```

## NAK Error Codes

Sahara defines 41 NAK error codes that the device may return at any time:

| Code | Description |
|------|-------------|
| Code | Description |
|------|-------------|
| 0x0300 | Sahara: success |
| 0x0301 | Sahara: invalid command |
| 0x0302 | Sahara: protocol mismatch |
| 0x0303 | Sahara: invalid target protocol |
| 0x0304 | Sahara: invalid host protocol |
| 0x0305 | Sahara: invalid packet size |
| 0x0306 | Sahara: unexpected packet |
| 0x0307 | Sahara: invalid transfer mode |
| 0x0308 | Sahara: invalid host ID |
| 0x0309 | Sahara: receive timeout |
| 0x030A | Sahara: transmit timeout |
| 0x030B | Sahara: invalid mode |
| 0x030C | Sahara: invalid host request |
| 0x030D | Sahara: read data error |
| 0x030E | Sahara: write data error |
| 0x030F | Sahara: invalid memory table |
| 0x0310 | Sahara: invalid memory info |
| 0x0311 | Sahara: memory debug not supported |
| 0x0312 | Sahara: memory read failed |
| 0x0313 | Sahara: invalid image ID |
| 0x0314 | Sahara: image not found |
| 0x0315 | Sahara: image authentication failed |
| 0x0316 | Sahara: image too large |
| 0x0317 | Sahara: authentication failed |
| 0x0318 | Sahara: invalid image header |
| 0x0319 | Sahara: invalid image header version |
| 0x031A | Sahara: invalid signature |
| 0x031B | Sahara: invalid hash |
| 0x031C | Sahara: fatal error |
| 0x031D | Sahara: command execution failure |
| 0x031E | Sahara: image decryption failed |
| 0x031F | Sahara: image header auth failed |
| 0x0320 | Sahara: image header version failed |
| 0x0321 | Sahara: image header MAC failed |
| 0x0322 | Sahara: image header MAC not found |
| 0x0323 | Sahara: image header signature failed |
| 0x0324 | Sahara: image header signature not found |
| 0x0325 | Sahara: invalid image content |
| 0x0326 | Sahara: invalid programmer |
| 0x0327 | Sahara: programmer auth failed |
| 0x0328 | Sahara: programmer mismatch |

## Implementation

The Sahara implementation in MBootCore consists of:

- **SaharaPacketSerializer** â€” serializes typed packets to raw byte buffers
- **SaharaPacketParser** â€” parses raw byte buffers into typed packets
- **SaharaStateMachine** â€” manages protocol state transitions (V2/V3)
- **SaharaProtocol** â€” full protocol implementation with handshake, upload, reset
- **SaharaAdapter** â€” bridges SaharaProtocol to IFlashDevice for the generic flash layer

All 19 packet types are supported with round-trip serialization/parsing verified through golden vector tests. Fuzz testing validates robustness against 500+ random and corrupt inputs.

