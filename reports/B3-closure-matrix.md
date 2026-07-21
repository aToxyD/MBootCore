# Campaign B3 — Closure Matrix

## Summary

| Status | Count |
|--------|-------|
| **Closed** | 18 |
| **Intentionally Deferred** | 3 |
| **Total** | 21 |

**No findings remain open.**

---

## Closure Details

| ID | Finding | Severity | Status | Resolution | Commit |
|----|---------|----------|--------|------------|--------|
| B2.1-001 | Qt6 vs Qt 6 | Medium | **Closed** | Standardized to "Qt 6" in prose (7 occurrences, 5 files) | `67ab102` |
| B2.1-002 | MbedTLS vs Mbed TLS | Medium | **Closed** | Standardized to "Mbed TLS" in prose (8 occurrences, 6 files) | `67ab102` |
| B2.1-003 | LoaderManager vs LoaderFramework | High | **Closed** | Added LoaderFramework alongside LoaderManager in AGENTS.md:71 | `67ab102` |
| B2.1-004 | CMake 3.20 vs 3.23 | Medium | **Closed** | AGENTS.md:409 updated to "3.20+ (3.23+ for presets)" | `67ab102` |
| B2.1-005 | GCC 9 vs 11 minimum | High | **Closed** | PlatformSupport.md:67 updated to "GCC 11+" | `67ab102` |
| B2.1-006 | Clang 6/12/14/15 contradiction | High | **Closed** | CompilerSupport.md table updated to Clang 15 | `67ab102` |
| B2.1-007 | Spreadtrum vs UNISOC enum | High | **Deferred** | Code has both as separate enum values (`UNISOC=3`, `Spreadtrum=6`). Docs correctly reflect both. | N/A |
| B2.1-008 | Qualcomm casing | Medium | **Closed** | FirmwarePackage.md:40 "QUALCOMM" → "Qualcomm" | `67ab102` |
| B2.1-009 | SecurityTesting.md compiler versions | Medium | **Closed** | Updated to GCC 11+, Clang 15+ | `a972758` |
| B2.1-010 | stats vs statistics alias | Candidate | **Closed** | CLI.md updated to show both `statistics` / `stats` | `a972758` |
| B2.2-001 | Broken link Build.md → BUILD_OPTIONS.md | High | **Closed** | Fixed to `BuildOptions.md` | `67ab102` |
| B2.2-002 | Broken link CompilerSupport.md → BUILD_OPTIONS.md | High | **Closed** | Fixed to `BuildOptions.md` | `67ab102` |
| B2.2-003 | Broken ref SecurityTesting → SecurityAuditReport | High | **Closed** | Updated paths to `docs/internal/SecurityAuditReport.md` | `a972758` |
| B2.2-004 | Wrong path IProtocolPlugin.hpp | High | **Closed** | Fixed in Contributing.md and Overview.md to `lib/include/mbootcore/plugin/` | `67ab102` |
| B2.2-005 | Wrong implied path IVendorPlugin.hpp | Candidate | **Deferred** | Contributing.md uses class name only, no file path implied. Not applicable. | N/A |
| B2.2-006 | 30 orphan documents | Candidate | **Closed** | Added 15+ cross-references across 16 files | `0e00423` |
| B2.3-009 | Spreadtrum vs UNISOC in docs | Medium | **Deferred** | Same as B2.1-007. Both names exist in code; docs are correct. | N/A |
| B2.3-010 | stats vs statistics alias | Candidate | **Closed** | Same as B2.1-010. CLI.md updated. | `a972758` |
| B2.4-001 | Unannotated code blocks | Low | **Closed** | Added `txt` annotations to 5 ASCII diagram blocks | `a972758` |
| B2.4-002 | Long paragraphs | Candidate | **Deferred** | No paragraphs exceed 200 words. Not applicable. | N/A |
| B2.3-005 | Settings dialog undocumented | Candidate | **Closed** | Added Settings section to Studio.md | `0e00423` |

---

## Intentionally Deferred (3 findings)

| ID | Finding | Justification |
|----|---------|---------------|
| B2.1-007 | Spreadtrum vs UNISOC enum | `DeviceTypes.hpp` defines both `UNISOC = 3` and `Spreadtrum = 6` as separate enum values. Both names are correct — they represent different vendor identifiers. |
| B2.2-005 | Wrong implied path IVendorPlugin.hpp | Contributing.md:174 uses `IVendorPlugin` as a class name (not a file path). No path is implied. |
| B2.4-002 | Long paragraphs | No paragraphs exceed 200 words. The longest paragraphs are in DesignDecisions.md (180 words) and are well-structured. |

---

## Commits

| Commit | Description | Files |
|--------|-------------|-------|
| `67ab102` | B3 Patch 1: Critical documentation corrections | 12 files |
| `a972758` | B3 Patch 2: Source alignment and code block annotations | 5 files |
| `0e00423` | B3 Patch 3: Editorial quality — cross-references, Settings dialog | 16 files |

**Total: 3 commits, 28 file modifications (with overlap)**
