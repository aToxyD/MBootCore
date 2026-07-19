# Vendor Maturity Model

**Document Version:** 1.0
**Last Updated:** 2026-07-11

> This document defines the formal maturity states for every supported vendor.
> All criteria are objective and measurable. No subjective wording.

---

## Maturity States

Every vendor in MBootCore must be in exactly one of the following states.

### Experimental

Definition: The vendor has an enum entry and no protocol implementation.

Exit criteria to reach Scaffold:

| # | Criterion | Measured by |
|---|-----------|-------------|
| E1 | `VendorFamily` enum entry exists | Compile-time check |
| E2 | `VendorInfo` struct can be constructed | Unit test |
| E3 | Vendor can be registered in `VendorRegistry` | Unit test |
| E4 | Wire format types are defined (at least Probe + Handshake) | Header exists |
| E5 | At least 1 golden vector exists for Probe | Test file present |

### Scaffold

Definition: The vendor has a compilable protocol implementation gated behind
`MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`. It compiles and runs against synthetic
vectors only. It is not tested against real hardware.

Exit criteria to reach Preview:

| # | Criterion | Measured by |
|---|-----------|-------------|
| S1 | All Experimental criteria met | Audit |
| S2 | Protocol implementation compiles with zero warnings | `cmake --build` with `-Werror` |
| S3 | At least 5 golden vector round-trip tests pass | CTest output |
| S4 | Handshake sequence completes against synthetic transport | CTest `[scaffold]` |
| S5 | Encoder produces wire-format bytes matching spec | Golden vector byte comparison |
| S6 | Decoder correctly rejects malformed input | Fuzz or negative test |
| S7 | `IProtocolFactory::create()` returns non-null | Unit test |
| S8 | `IProtocolSession::open()` succeeds against virtual transport | Integration test |
| S9 | Zero memory leaks under ASan | `ctest -L Sanitizers` |
| S10 | Scaffold is isolated: no production code path depends on scaffold | `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=OFF` build succeeds |

### Preview

Definition: The vendor has been validated against at least one real hardware
device. Flashing may not be fully functional.

Exit criteria to reach Production:

| # | Criterion | Measured by |
|---|-----------|-------------|
| P1 | All Scaffold criteria met | Audit |
| P2 | At least 1 real device detected via USB enumeration | Hardware test log |
| P3 | Handshake completes against real device | Hardware test log |
| P4 | At least 3 command sequences succeed against real device | Hardware test log |
| P5 | At least 10 golden vectors from real device captures | Test file |
| P6 | Zero crashes across 100 synthetic sessions | Stress test |
| P7 | Latency < 500ms for handshake round-trip | Benchmark |
| P8 | Documentation covers supported devices, limitations, and known issues | `docs/vendor/` present |
| P9 | Scaffold tests run locally on every change | Local verification |

### Production

Definition: The vendor has been validated against multiple real devices.
Flashing sessions succeed reliably.

Exit criteria (all must be satisfied):

| # | Criterion | Measured by |
|---|-----------|-------------|
| Q1 | All Preview criteria met | Audit |
| Q2 | At least 3 distinct device models detected and flashed | Hardware test reports |
| Q3 | At least 20 golden vectors from real device captures | Test file |
| Q4 | Zero failures across 1000 synthetic sessions | Stress test |
| Q5 | Fuzz coverage > 1000 unique paths per protocol command | Fuzz report |
| Q6 | Regression test suite passes: 100% green | CTest `[vendor]` |
| Q7 | Session success rate > 99% against known-good devices | Hardware test log |
| Q8 | Flash success rate > 99% for supported partition layouts | Hardware test log |
| Q9 | Performance: full flash of 1GB image < 120 seconds | Benchmark |
| Q10 | Documentation: API reference, device matrix, troubleshooting | `docs/vendor/` |
| Q11 | Hardware-in-the-loop testing validated | Local verification |
| Q12 | Scaffold flag does not affect this vendor when OFF | `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS=OFF` build |

---

## Current Vendor States

| Vendor | Family | State | Rationale |
|--------|--------|-------|-----------|
| Qualcomm Sahara | Qualcomm | Production | Full state machines, real hardware validated, 20+ golden vectors |
| Qualcomm Firehose | Qualcomm | Production | Full XML engine, real hardware validated, streaming support |
| MediaTek BROM | MediaTek | Scaffold | Compiles, wire format defined, synthetic vectors only, no hardware |
| UNISOC FDL | UNISOC | Scaffold | Compiles, wire format defined, synthetic vectors only, no hardware |

---

## Rules

1. Every vendor must have exactly one maturity state.
2. A vendor may only advance (never regress) unless explicitly demoted by
   architectural review.
3. Advancement requires all criteria for the target state to be met.
4. Advancement must be reflected in `VendorInfo::maturity`.
5. Scaffold vendors are gated behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`.
   Production vendors are always compiled.
