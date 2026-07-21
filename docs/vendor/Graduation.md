# Vendor Graduation Checklists

**Document Version:** 1.0
**Last Updated:** 2026-07-11

> Per-vendor graduation checklists. Each checklist tracks remaining work
> from current state to Production.

---

## Qualcomm Sahara

**Current State:** Production

### Completed

- [x] Protocol state machine (11 states)
- [x] Packet serialization/deserialization (binary)
- [x] Read, Write, Execute, Command modes
- [x] Memory Debug support
- [x] Chip ID v2/v3 support
- [x] 20+ golden vectors (real device captures)
- [x] Golden vector round-trip tests
- [x] Golden vector byte-exact verification
- [x] Fuzz testing (500+ iterations)
- [x] Virtual session tests
- [x] Hardware detection and session tests
- [x] Stress testing (100 sessions)
- [x] Documentation

### Remaining Work

| # | Item | Priority | Notes |
|---|------|----------|-------|
| QS1 | Additional device models (SDX55, SDX62) | Medium | Currently SDX55 validated |
| QS2 | Performance benchmarking against spec | Low | Latency measurements |
| QS3 | Memory leak validation under ASan | Medium | Part of local sanitizer runs |

### Known Limitations

- EDL mode required on target device
- USB enumeration depends on VID/PID matching (05C6:9008)
- Some devices require specific loader images

### Required Tests

| Test | Status | Coverage |
|------|--------|----------|
| Golden vectors | Passing (18 sections) | All packet types |
| Fuzz | Passing | 500+ iterations |
| Virtual session | Passing | Full handshake cycle |
| Hardware | Passing (auto-skip) | 5 sections |
| State machine | Passing | All transitions |
| Stress | Passing | 100 sessions |

---

## Qualcomm Firehose

**Current State:** Production

### Completed

- [x] XML command engine (12+ command types)
- [x] Response parsing (ACK/NAK)
- [x] Memory read/write operations
- [x] Storage info queries
- [x] SHA-256 digest verification
- [x] Patch/Poke/Peek operations
- [x] 14 golden vector round-trip tests
- [x] Fuzz testing (500+ iterations)
- [x] Virtual session tests
- [x] Streaming mode support
- [x] Documentation

### Remaining Work

| # | Item | Priority | Notes |
|---|------|----------|-------|
| QF1 | Device tree overlay flashing | Low | Niche use case |
| QF2 | Multi-image batch flashing | Medium | Sequential works today |
| QF3 | Progress callback from device | Low | Polling-based today |

### Known Limitations

- Requires Sahara handshake first (Firehose is a post-handshake protocol)
- XML parsing overhead for high-throughput operations
- MaxPayloadSize negotiation required per device

### Required Tests

| Test | Status | Coverage |
|------|--------|----------|
| Golden vectors | Passing (14 sections) | All command types |
| Fuzz | Passing | 500+ iterations |
| Virtual session | Passing | Full command cycle |
| XML parsing | Passing | 12 command types + 2 response types |
| Stress | Passing | Concurrent sessions |

---

## MediaTek BROM

**Current State:** Scaffold

### Completed

- [x] Wire format types defined (Probe, Handshake, GetVersion, GetHwCode)
- [x] Protocol scaffold implementation (encoder, decoder, session, discovery, factory)
- [x] Scaffold gated behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`
- [x] Compiles with zero warnings

### Remaining Work

| # | Item | Priority | Priority |
|---|------|----------|----------|
| MT1 | Complete handshake sequence implementation | High | Required for Preview |
| MT2 | Add memory read/write commands | High | Required for Preview |
| MT3 | Add flash commands | High | Required for Preview |
| MT4 | Create 10+ golden vectors | High | Required for Preview |
| MT5 | Hardware detection via USB VID/PID | High | Required for Preview |
| MT6 | Real device testing | High | Required for Preview |
| MT7 | Fuzz testing | Medium | Required for Preview |
| MT8 | Stress testing | Medium | Required for Preview |
| MT9 | Performance benchmarking | Low | Required for Production |
| MT10 | Multi-device model support | Low | Required for Production |

### Known Limitations

- Only Probe and basic handshake commands implemented
- No real hardware testing
- Synthetic vectors only
- Wire format not validated against real devices

### Required Tests (Current)

| Test | Status | Coverage |
|------|--------|----------|
| Scaffold compilation | Passing | Zero warnings |
| Wire format types | Defined | Header only |

### Required Tests (Target)

| Test | Status | Needed for |
|------|--------|------------|
| Golden vectors (5+) | Not started | Scaffold graduation |
| Handshake sequence | Not started | Preview |
| Memory operations | Not started | Preview |
| Flash operations | Not started | Production |
| Hardware detection | Not started | Preview |
| Fuzz | Not started | Preview |
| Stress | Not started | Production |

---

## UNISOC FDL

**Current State:** Scaffold

### Completed

- [x] Wire format types defined (Probe, Handshake, GetVersion, GetChipInfo)
- [x] Protocol scaffold implementation (encoder, decoder, session, discovery, factory)
- [x] Scaffold gated behind `MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS`
- [x] Compiles with zero warnings

### Remaining Work

| # | Item | Priority | Priority |
|---|------|----------|----------|
| UF1 | Complete handshake sequence implementation | High | Required for Preview |
| UF2 | Add memory read/write commands | High | Required for Preview |
| UF3 | Add flash commands | High | Required for Preview |
| UF4 | Create 10+ golden vectors | High | Required for Preview |
| UF5 | Hardware detection via USB VID/PID | High | Required for Preview |
| UF6 | Real device testing | High | Required for Preview |
| UF7 | Fuzz testing | Medium | Required for Preview |
| UF8 | Stress testing | Medium | Required for Preview |
| UF9 | Performance benchmarking | Low | Required for Production |
| UF10 | Multi-device model support | Low | Required for Production |

### Known Limitations

- Only Probe and basic handshake commands implemented
- No real hardware testing
- Synthetic vectors only
- Wire format not validated against real devices

### Required Tests (Current)

| Test | Status | Coverage |
|------|--------|----------|
| Scaffold compilation | Passing | Zero warnings |
| Wire format types | Defined | Header only |

### Required Tests (Target)

| Test | Status | Needed for |
|------|--------|------------|
| Golden vectors (5+) | Not started | Scaffold graduation |
| Handshake sequence | Not started | Preview |
| Memory operations | Not started | Preview |
| Flash operations | Not started | Production |
| Hardware detection | Not started | Preview |
| Fuzz | Not started | Preview |
| Stress | Not started | Production |
