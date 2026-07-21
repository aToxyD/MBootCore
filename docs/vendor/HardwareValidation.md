# Hardware Validation Process

**Document Version:** 1.0
**Last Updated:** 2026-07-11

> Process for validating vendor implementations against real hardware.

---

## Framework

Hardware tests use the `HardwareDetection` utility in
`tests/hardware/HardwareDetection.hpp` to:

1. Enumerate connected USB devices
2. Match VID/PID against known vendor tables
3. Skip tests gracefully when no device is found
4. Log test environment details

### Auto-Skip Behavior

All hardware tests use the `MBOOT_REQUIRE_HARDWARE` macro which calls
Catch2's `SKIP()` when no matching device is detected. This ensures:

- Tests pass locally (no hardware available)
- Tests run on developer machines with hardware
- No false failures from missing devices

### Test Execution

```bash
# Run hardware tests (auto-skip if no device)
ctest -L Hardware

# Run specific vendor hardware test
ctest -R qualcomm_hardware_test

# Run with verbose output
ctest -R qualcomm_hardware_test -V
```

## Vendor Test Stubs

| Vendor | Test File | Status | Skip Behavior |
|--------|-----------|--------|---------------|
| Qualcomm | `qualcomm_hardware_test.cpp` | Implemented | Auto-skips if no Qualcomm device |
| MediaTek | `mediatek_hardware_test.cpp` | Stub | Auto-skips (always) |
| UNISOC | `unisoc_hardware_test.cpp` | Stub | Auto-skips (always) |

## Test Categories

| Category | Description | Example |
|----------|-------------|---------|
| Detection | Enumerate USB devices, match VID/PID | `test_deviceDetection` |
| Handshake | Complete protocol handshake | `test_saharaHello` |
| Commands | Execute protocol-specific commands | `test_firehoseGetStorageInfo` |
| Flash | Write data to device | `test_fullFlash` (future) |
| Stress | Repeated connect/disconnect cycles | `test_repeatedSessions` (future) |

## Environment Reporting

Each hardware test reports:

- OS and version
- USB controller availability
- libusb version
- Connected device list
- Transport statistics
- Test duration

This information is captured in CTest output for local debugging.

## Local Verification

Hardware tests are labeled `Hardware` in CTest and are always skipped when
no physical devices are connected. Local verification validates that:

1. Hardware test binaries compile
2. Hardware test stubs integrate correctly
3. Skip logic works as expected
4. No hardware test blocks the release pipeline
