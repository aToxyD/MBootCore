# Generic Flash Abstraction Layer

## Overview

The Generic Flash Abstraction Layer sits between protocol-specific code and the application. It provides a unified interface for flash operations regardless of the underlying protocol (Sahara, Firehose, or future protocols).

## Architecture

```
Application (Session / User Code)
          │
          ▼
┌─────────────────────────────────────────┐
│         IFlashDevice (Generic)           │
│  - open / close / isOpen                │
│  - capabilities()                       │
│  - deviceInfo()                         │
│  - getStorageInfo() / getPartitions()   │
│  - readMemory / writeMemory / eraseMemory│
│  - readPartition / writePartition / ... │
│  - uploadLoader / reset / powerReset    │
│  - cancel / setProgressCallback         │
└─────────────────────────────────────────┘
          ▲
          │  implements
┌─────────┴────────────┬──────────────────┐
│   SaharaAdapter      │  FirehoseAdapter │  ← Protocol Adapters
│   (EDL mode)         │  (flash ops)     │
└──────────────────────┴──────────────────┘
          ▲                         ▲
          │                         │
┌─────────┴──────────┐  ┌───────────┴──────────┐
│  SaharaProtocol    │  │  FirehoseProtocol    │  ← Protocol implementations
│  - handshake       │  │  - handshake          │
│  - uploadProgrammer│  │  - program / read     │
│  - reset           │  │  - erase / reset      │
└────────────────────┘  └──────────────────────┘
          ▲                         ▲
          └────────┬────────────────┘
                   │
          ┌────────┴────────┐
          │   ITransport    │  ← USB / Serial / TCP
          └─────────────────┘
```

## File Structure

```
lib/include/mbootcore/generic/
├── FlashCapability.hpp     — 18-bit capability enum
├── DeviceInfo.hpp          — GenericDeviceInfo, BootMode enum
├── StorageInfo.hpp         — StorageInfo, StorageType enum
├── PartitionModel.hpp      — PartitionEntry, PartitionTable
├── ProgressInfo.hpp        — ProgressInfo (total/transferred/speed/ETA/percentage)
├── IFlashOperation.hpp     — Operation interface with requiredCapabilities()
├── IFlashDevice.hpp        — Main generic device interface (20 methods)
├── OperationPipeline.hpp   — Pipeline: validate → capabilities → execute
├── ErrorMapping.hpp        — Protocol error → generic error mapping
│
└── adapter/
    ├── SaharaAdapter.hpp   — IFlashDevice via Sahara
    └── FirehoseAdapter.hpp — IFlashDevice via Firehose
```

## IFlashDevice Interface

```cpp
class IFlashDevice {
    virtual Result<Unit> open();
    virtual Result<Unit> close();
    virtual bool isOpen() const;

    virtual FlashCapability capabilities() const;
    virtual Result<DeviceInfo> deviceInfo() const;
    virtual Result<StorageInfo> getStorageInfo() const;
    virtual Result<std::vector<PartitionEntry>> getPartitions() const;

    virtual Result<ByteBuffer> readMemory(uint64_t address, size_t size);
    virtual Result<Unit> writeMemory(uint64_t address, const ByteBuffer& data);
    virtual Result<Unit> eraseMemory(uint64_t address, size_t size);

    virtual Result<ByteBuffer> readPartition(std::string_view name);
    virtual Result<Unit> writePartition(std::string_view name, const ByteBuffer& data);

    virtual Result<Unit> uploadLoader(const ByteBuffer& loaderData);
    virtual Result<Unit> reset();
    virtual Result<Unit> powerReset();

    virtual void cancel();
    virtual void setProgressCallback(ProgressCallback callback);
};
```

## Capability System

The `FlashCapability` bit-flag enum provides runtime feature detection:

| Capability | SaharaAdapter | FirehoseAdapter |
|-----------|:-------------:|:---------------:|
| Read | ✗ | ✓ |
| Write | ✗ | ✓ |
| Erase | ✗ | ✓ |
| Reset | ✓ | ✓ |
| UploadLoader | ✓ | ✗ |
| StorageInfo | ✗ | ✓ |
| Partitions | ✗ | ✗ |
| Peek | ✗ | ✓ |
| Poke | ✗ | ✓ |
| Patch | ✗ | ✓ |
| PowerReset | ✗ | ✓ |
| Sha256Digest | ✗ | ✓ |

## Protocol Adapters

### SaharaAdapter

Bridges `SaharaProtocol` to `IFlashDevice`. Supports:
- Upload programmer (chunked READ_DATA)
- Reset (DONE_REQ → RESET sequence)
- Capabilities limited to EDL mode operations

### FirehoseAdapter

Bridges `FirehoseProtocol` to `IFlashDevice`. Supports:
- All flash operations (read, write, erase)
- Storage information retrieval
- Peek/poke/patch for memory access
- Power management and reset
- SHA-256 digest

## Operation Pipeline

Each flash operation follows a three-stage pipeline:

```
IFlashOperation::execute()
    ↓
validate() → returns error if invalid
    ↓
checkCapabilities() → returns NotSupported if device can't perform operation
    ↓
executeImpl() → runs operation with progress callback
    ↓
Result (Ok or Error)
```

## Error Mapping

All adapters return existing `ErrorCode` values:
- `Success` — operation completed
- `NotSupported` — device doesn't support this operation
- `DeviceDisconnected` — transport lost during operation
- `TransportError` — underlying transport failure
- `ProtocolError` — protocol-level NAK or malformed response
- `Cancelled` — operation cancelled by user

Protocol-specific details are logged via the logger but abstracted at the generic API level.

## Extensibility

Adding support for a new protocol requires:
1. Create `lib/include/mbootcore/generic/adapter/NewProtocolAdapter.hpp`
2. Create `lib/src/generic/adapter/NewProtocolAdapter.cpp`
3. Implement `IFlashDevice` interface
4. Return appropriate `FlashCapability` flags
5. Register with discovery protocol registry

No changes to the generic layer interfaces are required.
