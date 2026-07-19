# Device Discovery & Protocol Negotiation

## Overview

The Device Discovery and Protocol Negotiation framework is the entry point of MBootCore. It discovers devices, identifies BootROM/download mode, determines vendor and protocol, and builds the correct boot pipeline — all without any protocol-specific code in the generic layer.

### Design Goals

- **Zero protocol knowledge in generic layer**: No Qualcomm, Sahara, Firehose, MediaTek, UNISOC headers
- **Registry-based**: Protocols register themselves; no switches or if-else chains
- **Scoring-based negotiation**: Confidence scores determine best protocol match
- **Testable without hardware**: Virtual device detector simulates any platform
- **Clean Architecture**: SOLID, DI, RAII, move semantics

## Architecture

```
Application (Session)
    │
    ▼
DeviceDiscoveryEngine
    │
    ├── ProtocolRegistry
    │   ├── IDeviceDetector[]      ← enumerates/identifies/probes
    │   ├── IProtocolNegotiator[]  ← scores descriptors against protocol
    │   └── IProtocolFactory[]     ← creates IFlashDevice + BootPipeline
    │
    ├── ProtocolNegotiationEngine
    │   └── bestMatch() → NegotiationResult{ProtocolType, confidence, reason}
    │
    └── VirtualDeviceDetector (for testing)
```

### Flow

```
User calls Session::discover()
    │
    ▼
DeviceDiscoveryEngine::discoverAll()
    │  for each detector:
    │    enumerate() → candidates
    │    probe()     → fill descriptors
    │    identify()  → refine match
    ▼
Result: std::vector<DeviceDescriptor>
    │
    ▼
ProtocolNegotiationEngine::negotiate()
    │  for each negotiator:
    │    score(descriptor) → confidence
    ▼
Result: NegotiationResult
    │
    ▼
ProtocolFactory::create(descriptor)
    │
    ▼
IFlashDevice + BootPipeline
```

## Components

### DiscoveryTypes

| Type | Description |
|------|-------------|
| `Vendor` | Enum: Unknown, Qualcomm, MediaTek, UNISOC, Samsung, Rockchip, Spreadtrum, Apple, Google, Huawei, Custom |
| `BootMode` | Enum: Unknown, BootROM, EDL, Firehose, Fastboot, ADB, Recovery, DownloadMode, Preloader, BROM, Custom |
| `TransportType` | Enum: Unknown, USB, Serial, TCP, Virtual |
| `ProtocolType` | Enum: Unknown, Sahara, Firehose, Fastboot, MediaTekBROM, MediaTekDA, UNISOCBootROM, UNISOCFDL, USBStream, Custom |
| `DeviceDescriptor` | Vendor, boot mode, transport type, VID/PID, serial, friendly name, chip IDs, capabilities |
| `DiscoveryResult` | List of discovered device descriptors |
| `NegotiationResult` | Matched protocol type, confidence score, reason string |

### IDeviceDetector

Interface for device enumeration, identification, and probing:

```cpp
class IDeviceDetector {
    Result<std::vector<DeviceDescriptor>> enumerate();
    Result<DeviceDescriptor> identify(DeviceDescriptor);
    Result<DeviceDescriptor> probe(DeviceDescriptor);
};
```

Implementations scan transport backends for connected devices, detect vendor-specific USB descriptors, and probe for protocol compatibility.

### IProtocolNegotiator

Interface for scoring devices against protocol capabilities:

```cpp
class IProtocolNegotiator {
    int score(const DeviceDescriptor& descriptor);
    ProtocolType protocolType() const;
};
```

Returns a confidence score (0–200+) indicating how well the descriptor matches the protocol. The highest-scoring negotiator wins.

**Scoring criteria:**
- PK Hash match: 80 points
- MSM ID match: 50 points
- Vendor match: 40 points
- Chipset match: 30 points
- Protocol match: 15 points
- Storage type match: 10 points

### IProtocolFactory

Creates the appropriate protocol stack from a device descriptor:

```cpp
class IProtocolFactory {
    Result<std::unique_ptr<IFlashDevice>> createFlashDevice(const DeviceDescriptor&);
    Result<std::unique_ptr<BootPipeline>> createPipeline(const DeviceDescriptor&);
    ProtocolType protocolType() const;
};
```

### ProtocolRegistry

Central registry that stores and resolves all detectors, negotiators, and factories:

```cpp
class ProtocolRegistry {
    void registerDetector(std::unique_ptr<IDeviceDetector>);
    void registerNegotiator(std::unique_ptr<IProtocolNegotiator>);
    void registerFactory(std::unique_ptr<IProtocolFactory>);

    IDeviceDetector* findDetector(std::string_view name);
    IProtocolNegotiator* findNegotiator(ProtocolType);
    IProtocolFactory* findFactory(ProtocolType);

    void clear();
};
```

### DeviceDiscoveryEngine

Orchestration layer that drives the discovery process:

```cpp
class DeviceDiscoveryEngine {
    DeviceDiscoveryEngine(ProtocolRegistry& registry);

    Result<std::vector<DeviceDescriptor>> discoverAll();
    Result<std::vector<DeviceDescriptor>> discoverByVendor(Vendor vendor);
    Result<DeviceDescriptor> probeDevice(DeviceDescriptor descriptor);
};
```

### ProtocolNegotiationEngine

Selects the highest-confidence protocol match:

```cpp
class ProtocolNegotiationEngine {
    ProtocolNegotiationEngine(ProtocolRegistry& registry);

    Result<NegotiationResult> negotiate(const DeviceDescriptor& descriptor);
    Result<std::vector<NegotiationResult>> negotiateAll(const DeviceDescriptor& descriptor);
    NegotiationResult bestMatch(const std::vector<NegotiationResult>& results);
};
```

### VirtualDeviceDetector

Simulates device detection for testing without hardware. Supports 8+ device configurations:

| Preset | Description |
|--------|-------------|
| `createQualcommEDL` | Sahara protocol (EDL) |
| `createQualcommFirehose` | Firehose protocol |
| `createMediaTekPreloader` | MediaTek Preloader |
| `createUNISOCBootROM` | UNISOC BootROM |
| `createSamsungDownload` | Samsung Download mode |
| `createRockchipMaskROM` | Rockchip MaskROM |
| `createUnknownDevice` | Generic unknown device |
| `createDisconnectedDevice` | Fails connection attempts |
| `createTimeoutDevice` | Fails due to timeouts |

Supports failure injection, hotplug simulation, and reconnection scenarios.
