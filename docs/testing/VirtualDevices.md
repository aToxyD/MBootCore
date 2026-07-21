# Virtual Devices

The virtual device system provides test and simulation capabilities without
requiring real hardware. It implements the same interfaces used by physical
device detection.

## VirtualDeviceDetector

A test implementation of `IDeviceDetector` that allows programmatic creation
of virtual device descriptors.

**Header:** `mbootcore/discovery/VirtualDeviceDetector.hpp`

### Adding Virtual Devices

```cpp
#include <mbootcore/MBootCore.hpp>

mbootcore::VirtualDeviceDetector detector;

// Add a Qualcomm EDL device
mbootcore::VirtualDeviceSpec spec;
spec.vendor = mbootcore::Vendor::Qualcomm;
spec.bootMode = mbootcore::BootMode::EDL;
spec.transport = mbootcore::TransportType::USB;
spec.usbVid = 0x05C6;
spec.usbPid = 0x9008;
spec.friendlyName = "Qualcomm EDL Device";
spec.connectionPath = "usb:0";

detector.addDevice(spec);
```

### VirtualDeviceSpec

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `vendor` | Vendor | — | Device vendor |
| `bootMode` | BootMode | — | Boot mode |
| `transport` | TransportType | — | Transport type |
| `protocolHint` | ProtocolType | — | Protocol hint |
| `usbVid` | uint16 | — | USB Vendor ID |
| `usbPid` | uint16 | — | USB Product ID |
| `friendlyName` | string | — | Human-readable name |
| `connectionPath` | string | — | Connection path |
| `connectable` | bool | `true` | Whether device can connect |
| `probeDelayMs` | int | `0` | Delay during probe |
| `failProbability` | double | `0.0` | Failure probability (0.0–1.0) |

### Preset Factory Methods

| Method | VID | PID | Description |
|--------|-----|-----|-------------|
| `createQualcommEDL()` | `0x05C6` | `0x9008` | Qualcomm EDL mode |
| `createQualcommFirehose()` | `0x05C6` | `0x900E` | Qualcomm Firehose |
| `createMediaTekPreloader()` | `0x0E8D` | `0x2000` | MediaTek Preloader |
| `createUNISOCBootROM()` | `0x1782` | `0x4D00` | UNISOC BootROM |
| `createSamsungDownload()` | `0x04E8` | `0x685D` | Samsung Download mode |
| `createRockchipMaskROM()` | `0x2207` | `0x350A` | Rockchip Mask ROM |
| `createUnknownDevice()` | — | — | Generic unknown device |
| `createDisconnectedDevice()` | — | — | Simulates disconnected device |
| `createTimeoutDevice()` | — | — | Simulates timeout |

### Controlling Failure Behavior

```cpp
// Set global failure probability
detector.setFailProbability(0.3);  // 30% chance of probe failure

// Set probe delay
detector.setProbeDelay(100);  // 100ms delay

// Set random seed for reproducible tests
detector.setRandomSeed(42);
```

### Device Management

| Method | Description |
|--------|-------------|
| `addDevice(spec)` | Register a virtual device |
| `removeDevice(path)` | Remove by connection path |
| `clearDevices()` | Remove all devices |
| `deviceCount()` | Number of registered devices |

## VirtualProtocolNegotiator

A test implementation of `IProtocolNegotiator` for protocol negotiation tests.

**Header:** `mbootcore/discovery/VirtualDeviceDetector.hpp`

```cpp
mbootcore::VirtualProtocolNegotiator negotiator(
    mbootcore::ProtocolType::Sahara, 100  // protocol, confidence
);
```

| Method | Description |
|--------|-------------|
| `name()` | Returns negotiator name |
| `negotiate(descriptor)` | Performs negotiation |
| `setConfidence(int)` | Sets confidence level |
| `confidence()` | Gets current confidence |

## See Also

- [Device Discovery](../architecture/DeviceDiscovery.md) — physical device detection
- [Testing](../testing/Testing.md) — test suite organization
