# Boot Flow

## Overview

The MBootCore boot pipeline orchestrates a sequence of stages that transition a device from power-on to a ready state where flash operations can be performed. Each stage is handled by a callback function — the pipeline coordinates sequencing, progress reporting, cancellation, and recovery.

## Pipeline Architecture

```
BootPipeline
  ├── BootContext          — shared mutable state (move-only)
  ├── RecoveryStrategy     — configurable failure recovery per stage
  ├── BootPipelineConfig   — timeouts, retries, flags
  ├── Stage handlers       — std::function per stage
  └── ProgressCallback     — reported on every stage change
```

The pipeline has zero protocol dependencies — no Sahara, Firehose, or ELF headers are included. Stage handlers bridge the pipeline to protocol implementations.

## Boot Stages

```
Stage 0:  Connect
    ↓
Stage 1:  Sahara Initialization (handshake + version negotiation)
    ↓
Stage 2:  Loader Selection (find matching boot programmer)
    ↓
Stage 3:  ELF Parsing (parse and validate programmer binary)
    ↓
Stage 4:  Programmer Upload (transfer via Sahara)
    ↓
Stage 5:  Programmer Execute (start on device)
    ↓
Stage 6:  Firehose Detection (detect Firehose XML interface)
    ↓
Stage 7:  Firehose Configuration (configure memory and transport)
    ↓
Stage 8:  Ready (device is ready for flash operations)
```

### Stage Details

**Connect**: Establish transport connection with the device. Detects USB descriptors, identifies vendor and boot mode.

**Sahara Initialization**: Performs Sahara protocol handshake. Negotiates protocol version (V2 or V3), exchanges chip identification information. The device sends a HELLO_REQ packet; the host responds with HELLO_RESP.

**Loader Selection**: Uses the Loader Framework to find a compatible boot programmer. Matching uses descending priority scoring: PK Hash → MSM ID → Vendor → Chipset → Protocol → Storage type.

**ELF Parsing**: Parses the selected loader binary (ELF format). Validates magic, header fields, segment alignment, and overlap detection. Builds the memory image from PT_LOAD segments.

**Programmer Upload**: Transfers the programmer binary to the device via Sahara's chunked READ_DATA commands. Reports progress per chunk.

**Programmer Execute**: Sends the DONE_REQ command and triggers device reset. The device executes the uploaded programmer and exposes a Firehose XML interface.

**Firehose Detection**: Confirms the device is now in Firehose mode. May re-establish transport connection if the reset changed USB descriptors.

**Firehose Configuration**: Sends `<configure>` command with memory type, max payload sizes, and zero-length packet awareness. Waits for ACK response (with retries for power-on delay).

**Ready**: Device is ready for flash operations through `IFlashDevice`.

## Recovery Strategy

Each stage can be configured with independent recovery rules:

| Strategy | Behavior |
|----------|----------|
| **Retry** | Re-attempt the stage up to N times with configurable delay |
| **Rollback** | Return to a previous stage and retry from there |
| **Abort** | Immediately fail the entire pipeline |

Recovery rules are defined per stage in `RecoveryStrategy`:
- `maxRetries` — maximum retry attempts per stage
- `retryDelay` — delay between retries (milliseconds)
- `rollbackStage` — stage index to roll back to on failure
- `action` — RecoveryAction::Retry, RecoveryAction::Rollback, or RecoveryAction::Abort

## Shared Context

`BootContext` carries state across all stages:
- `transport` — `ITransport` reference
- `deviceInfo` — `DeviceInfo` with vendor, boot mode, chip IDs
- `descriptor` — `DeviceDescriptor` from discovery
- `loaderPath` — path to selected loader binary
- `loaderData` — raw loader binary data
- `properties` — string-keyed metadata map

## Pipeline Factory

`BootPipelineFactory` creates configured pipeline instances:

```cpp
auto pipeline = BootPipelineFactory::createWithDefaults();
// or
auto pipeline = BootPipelineFactory::createFromDescriptor(descriptor);
```

The factory configures default stage handlers, recovery rules, and timeouts suitable for the detected device type.
