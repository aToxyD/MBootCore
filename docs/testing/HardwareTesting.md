# Hardware Testing

## Overview

MBootCore includes hardware validation infrastructure that automatically detects connected devices and runs appropriate tests. When no compatible hardware is available, tests skip cleanly with Catch2 `SKIP()` — they never fail due to missing hardware.

## Auto-Skip Policy

```cpp
// In every hardware test case:
MBOOT_REQUIRE_HARDWARE(HardwareDetection::isQualcommDevicePresent(), "Qualcomm");
// If no device → SKIP("Qualcomm hardware not connected") is called → test is reported as SKIP, not FAIL
```

- **No hardware**: all hardware tests SKIP (pass)
- **With hardware**: hardware tests run and must pass
- **Default ctest run**: hardware tests appear but SKIP

## Running Hardware Tests

```powershell
cd build

# Run all tests including hardware (skips if none connected)
ctest --output-on-failure

# Run only hardware tests
ctest -L Hardware --output-on-failure

# List hardware tests without running
ctest -L Hardware -N
```

## Supported Vendors

### Qualcomm (Primary)

| Attribute | Details |
|-----------|---------|
| Protocol | Sahara (BootROM) → Firehose (programmer) |
| USB VID | `0x05C6` (Qualcomm HS-USB) |
| USB PID | `0x9008` (EDL / Emergency Download Mode) |
| Connection | USB Type-A → USB Type-C cable |
| Entry | Hold Volume Down + Power, or `adb reboot edl` |
| Test file | `tests/hardware/qualcomm_hardware_test.cpp` |

### MediaTek (Scaffold — Not Production Ready)

| Attribute | Details |
|-----------|---------|
| Protocol | BROM (BootROM) → DA (Download Agent) |
| USB VID | `0x0E8D` (MediaTek) |
| USB PID | `0x0003` (BROM mode) |
| Connection | USB with BROM trigger (short TP to GND or power without battery) |
| Test file | None (scaffold — no hardware test available) |

### Samsung

| Attribute | Details |
|-----------|---------|
| Protocol | Odin Download Protocol (ODP) |
| USB VID | `0x04E8` (Samsung) |
| USB PID | `0x685D` (Download Mode) |
| Connection | Volume Down + Home + Power → Odin mode |

### UNISOC / Spreadtrum (Scaffold — Not Production Ready)

| Attribute | Details |
|-----------|---------|
| Protocol | SPRD BROM |
| USB VID | `0x1782` (Spreadtrum) |
| USB PID | `0x4D00` |
| Status | Reference scaffold — no hardware test available |

### Rockchip

| Attribute | Details |
|-----------|---------|
| Protocol | Rockchip Loader |
| USB VID | `0x2207` |
| USB PID | `0x350A` |

## Driver Requirements (Windows)

| Device | Driver | Tool |
|--------|--------|------|
| Qualcomm EDL (0x05C6:0x9008) | WinUSB | [Zadig](https://zadig.akeo.ie/) |
| MediaTek BROM (0x0E8D:0x0003) | WinUSB or VCOM | Zadig or MTK USB driver (scaffold — driver requirement not verified) |
| Generic USB device | WinUSB | Zadig |

## Test Design

Hardware tests validate:
1. USB transport detection and connection
2. Sahara Hello/Response handshake
3. Firehose configure/getStorageInfo
4. GPT read from real device
5. Backup/restore on test partition

## Verification Sequence

When physical hardware is available:

1. Install WinUSB driver via Zadig for the target device
2. Connect device in appropriate boot mode
3. Run `ctest -L Hardware` — tests execute against real device
4. Review results and update hardware test documentation

## See Also

- [Virtual Devices](VirtualDevices.md) — test without physical hardware
- [Security Testing](SecurityTesting.md) — sanitizer and fuzzing infrastructure
