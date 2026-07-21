# Scaffold Philosophy

**Document Version:** 1.0
**Last Updated:** 2026-07-11

> Why MediaTek and UNISOC remain Scaffold implementations.

---

## Purpose

MediaTek BROM and UNISOC FDL exist as **extensibility demonstrations**.
They prove that the Protocol Platform can accommodate new vendors
without modifying platform layers.

They are **not** production implementations.

## Why Scaffold

| Reason | Explanation |
|--------|-------------|
| **No hardware access** | We do not have MediaTek or UNISOC devices for testing. |
| **No spec documents** | The wire formats are reconstructed from open-source references, not vendor documentation. |
| **No validation** | Wire-format bytes have never been compared against real device traffic. |
| **No device matrix** | We cannot enumerate supported chipsets, boot modes, or partition layouts. |
| **Deliberate constraint** | Making these "Production" would require hardware we do not have, and claiming otherwise would be dishonest. |

## What Scaffold Proves

1. **Platform extensibility** — A new vendor can be added by implementing
   `IProtocol`, `IProtocolSession`, `IProtocolFactory`, `IProtocolDiscovery`.
2. **Zero platform modifications** — Adding MediaTek or UNISOC required zero
   changes to Transport, Runtime, or Protocol Platform.
3. **Gated compilation** — Scaffold code is excluded from the build when
   `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=OFF`, confirming isolation.
4. **Interface compliance** — The scaffold satisfies all interface contracts
   required by the Protocol Platform.

## What Would Be Required for Production

To graduate MediaTek or UNISOC from Scaffold to Production, the project would
need:

1. Physical hardware devices (multiple models)
2. Vendor documentation or reverse-engineered specs
3. USB VID/PID validation against real devices
4. Golden vectors captured from real device traffic
5. Flash success rate > 99% across supported devices
6. Dedicated hardware-in-the-loop testing
7. Architectural review and ADR if changing frozen layers

Without all of these, claiming "Production" status would be misleading.

## Gating

Scaffold implementations are gated behind:

```cmake
-DMBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=ON
```

Default is OFF. This ensures:

- Production builds never include scaffold code
- Scaffold compilation errors do not block production releases
- Scaffold test failures do not fail local verification for production vendors
