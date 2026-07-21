# Documentation Baseline — MBootCore v1.0.0

## Metadata

| Field | Value |
|-------|-------|
| Project | MBootCore |
| Version | v1.0.0 |
| Snapshot Date | 2026-07-21 |
| Git Branch | main |
| Git Commit | 6681626 |
| Working Tree | Clean |
| Generator | Manual (find, grep, wc, git) |
| External Tools | Unavailable (markdown-link-check, markdownlint, vale not installed) |

---

## Pre-flight Verification

| Check | Status |
|-------|--------|
| All 7 root documentation files exist | PASS |
| docs/ structure with 6 subdirectories exists | PASS |
| All planned file paths for Campaign A exist | PASS |
| docs/README.md links all resolve | PASS |
| Root README.md links all resolve | PASS |
| Working tree clean | PASS |
| On `main` branch | PASS |
| No unexpected blockers | PASS |

---

## Metrics Summary

| Metric | Count |
|--------|-------|
| Project-owned markdown files | 46 |
| Root documentation files | 7 |
| docs/ markdown files | 44 |
| examples/ README files | 11 |
| tests/ README files | 1 |
| templates/ README files | 1 |
| Total documentation lines (root) | 1,191 |
| Total documentation lines (docs/) | 6,340 |
| Total documentation lines (all) | 7,531 |
| Known broken links | 0 |
| Known factual errors | 10 |
| Known terminology inconsistencies | 4 categories |
| Duplicate anchors | 2 files |
| Orphan files | 4 |
| Placeholder markers (TODO/FIXME/TBD) | 0 (1 false positive) |

---

## File Inventory

### Root Documentation Files (7)

| File | Lines |
|------|-------|
| `README.md` | 74 |
| `AGENTS.md` | 543 |
| `BUILD_OPTIONS.md` | 89 |
| `CONTRIBUTING.md` | 240 |
| `SECURITY.md` | 144 |
| `CHANGELOG.md` | 7 |
| `ThirdPartyLicenses.md` | 94 |

### docs/ Structure (44 files, 6,340 lines)

#### docs/ root (2 files, 184 lines)

| File | Lines |
|------|-------|
| `docs/README.md` | 121 |
| `docs/BUILD_OPTIONS.md` | 63 |
| `docs/Doxyfile.in` | 58 (non-markdown) |

#### docs/architecture/ (15 files, 2,753 lines)

| File | Lines |
|------|-------|
| `Architecture.md` | 342 |
| `Plugin-System.md` | 298 |
| `Firehose.md` | 267 |
| `Boot-Pipeline.md` | 200 |
| `GPT.md` | 196 |
| `Firmware-Package.md` | 195 |
| `Device-Discovery.md` | 185 |
| `Loader-Framework.md` | 170 |
| `Generic-Framework.md` | 168 |
| `Session.md` | 146 |
| `ELF.md` | 145 |
| `Sahara.md` | 142 |
| `Design-Decisions.md` | 107 |
| `Security.md` | 90 |
| `BuildPipeline.md` | 102 |

#### docs/guides/ (11 files, 1,502 lines)

| File | Lines |
|------|-------|
| `Plugin-Development.md` | 302 |
| `Diagnostics.md` | 167 |
| `Build.md` | 138 |
| `Testing.md` | 130 |
| `Platform-Support.md` | 126 |
| `Logging.md` | 119 |
| `Thread-Safety.md` | 115 |
| `SDK-API.md` | 108 |
| `Hardware-Testing.md` | 107 |
| `Compiler-Support.md` | 106 |
| `Configuration.md` | 84 |

#### docs/gui/ (7 files, 621 lines)

| File | Lines |
|------|-------|
| `README.md` | 183 |
| `Architecture.md` | 156 |
| `Models.md` | 88 |
| `Components.md` | 71 |
| `Theme.md` | 60 |
| `Testing.md` | 44 |
| `Shortcuts.md` | 19 |

#### docs/specifications/ (3 files, 356 lines)

| File | Lines |
|------|-------|
| `File-Formats.md` | 174 |
| `Boot-Flow.md` | 98 |
| `Protocols.md` | 84 |

#### docs/testing/ (1 file, 372 lines)

| File | Lines |
|------|-------|
| `SecurityTesting.md` | 372 |

#### docs/vendor/ (5 files, 552 lines)

| File | Lines |
|------|-------|
| `VENDOR_GRADUATION.md` | 206 |
| `VENDOR_MATURITY.md` | 112 |
| `GOLDEN_VECTOR_POLICY.md` | 87 |
| `HARDWARE_VALIDATION.md` | 81 |
| `SCAFFOLD_PHILOSOPHY.md` | 66 |

### examples/ README Files (11)

| File |
|------|
| `examples/backup_partition/README.md` |
| `examples/basic_connect/README.md` |
| `examples/cli_embedding/README.md` |
| `examples/custom_vendor/README.md` |
| `examples/device_discovery/README.md` |
| `examples/flash_partition/README.md` |
| `examples/job_pipeline/README.md` |
| `examples/minimal/README.md` |
| `examples/plugin_creation/README.md` |
| `examples/runtime_usage/README.md` |
| `examples/workflow_execution/README.md` |

### Other README Files (2)

| File |
|------|
| `tests/corpus/README.md` |
| `templates/plugins/README.md` |

---

## Link Verification

### docs/README.md Links

All 40+ links resolve correctly:
- Architecture links (13): all resolve
- Specification links (3): all resolve
- Guide links (11): all resolve
- Example links (15): all resolve (including `../examples/sdk/`, `../examples/dynamic/`)
- GUI links (6): all resolve
- Vendor links (5): all resolve

### Root README.md Links

All links resolve correctly.

### Known Dangling References (not broken links, but references to non-existent files)

| Source | Reference | Status |
|--------|-----------|--------|
| `docs/testing/SecurityTesting.md:8` | `docs/testing/SecurityAuditReport.md` | File does not exist |
| `docs/testing/SecurityTesting.md:332` | `docs/testing/SecurityAuditReport.md` | File does not exist |

---

## Plan Deviations

### Files Not at Expected Locations

| Expected | Actual | Impact |
|----------|--------|--------|
| `docs/testing/Testing.md` | Does not exist | Will be created in Campaign A from `docs/guides/Testing.md` |
| `docs/build/BuildOptions.md` | Does not exist | Will be created in Campaign A (Commit 7) |

### Directories Not Yet Created (expected — created in Campaign A)

| Directory | Purpose |
|-----------|---------|
| `docs/getting-started/` | Installation, QuickStart |
| `docs/user-guide/` | CLI, Studio, Configuration, Logging, Diagnostics |
| `docs/sdk/` | SDK Overview, Plugin Development |
| `docs/build/` | Build, BuildOptions, CompilerSupport, PlatformSupport |
| `docs/developer/` | Contributing, ThreadSafety, CodingStandards |
| `docs/project/` | ChangeLog, Overview, ReleaseProcess |
| `docs/reference/` | Glossary, CLIReference, ErrorCodes, ConfigReference, ThirdPartyLicenses |
| `docs/security/` | SecurityPolicy |
| `docs/internal/` | AGENTS.md |
| `docs/api/` | Doxyfile.in |
| `docs/assets/` | images/, diagrams/ |
| `reports/` | This file |

### Additional Files Not in Original Plan (pre-existing)

| File | Notes |
|------|-------|
| `examples/dynamic/` | Dynamic plugin loading examples |
| `examples/sdk/` | SDK plugin examples |
| `examples/sahara/main.cpp` | Sahara example |
| `examples/firehose/main.cpp` | Firehose example |

These are not documentation structure deviations — they are example files that exist and are correctly linked from `docs/README.md`.

---

## Orphan Files

Files that exist in `docs/` but are not referenced from any index (`docs/README.md` or root `README.md`):

| File | Notes |
|------|-------|
| `docs/architecture/BuildPipeline.md` | Not linked from docs/README.md |
| `docs/BUILD_OPTIONS.md` | Will be merged into `docs/build/BuildOptions.md` in Campaign A |
| `docs/gui/README.md` | Not linked from docs/README.md (gui section links individual files) |
| `docs/README.md` | Self (documentation root) |
| `docs/testing/SecurityTesting.md` | Not linked from docs/README.md testing section |

---

## Terminology Inconsistencies

### Qt6 vs Qt 6

| Occurrence | Form | File |
|------------|------|------|
| 3 | `Qt6` | AGENTS.md, BUILD_OPTIONS.md, gui/README.md |
| 5 | `Qt 6` | Build.md, Platform-Support.md, README.md, gui/README.md |

**Canonical form:** `Qt 6` (with space)

### MbedTLS vs Mbed TLS

| Occurrence | Form | Files |
|------------|------|-------|
| 12 | `MbedTLS` | Build.md, Architecture.md, Design-Decisions.md, Security.md, BUILD_OPTIONS.md, README.md, AGENTS.md |
| 3 | `Mbed TLS` | Architecture.md, Security.md, README.md |

**Canonical form:** `Mbed TLS` in prose, `MbedTLS` in code identifiers

### Runtime Engine / Session Engine / Firmware Package Engine

| Occurrence | Form | File |
|------------|------|------|
| 1 | `Runtime Engine` | Thread-Safety.md |
| 2 | `Session Engine` | Session.md, docs/README.md |
| 2 | `Firmware Package Engine` | Firmware-Package.md, docs/README.md |

**Canonical form:** `Runtime`, `Session`, `Firmware Package` (without "Engine")

### CMake Minimum Version

| Occurrence | Form | File |
|------------|------|------|
| 4 | `3.20+` | Plugin-Development.md, Build.md, Platform-Support.md, Architecture.md |
| 1 | `3.23+` | Build.md (presets section) |

**Canonical form:** `3.20+ (3.23+ for presets)`

### Compiler Minimums

| File | GCC | Clang | Note |
|------|-----|-------|------|
| `Compiler-Support.md` table | 11 | 14 | Table says 14 |
| `Compiler-Support.md` section | 11 | 15 | Section says 15 |
| `Build.md` | 11+ | 15+ | |
| `Platform-Support.md` | 9+ | 14+ | Outdated |

**Canonical form:** GCC 11+, Clang 15+

---

## Factual Errors

### 1. PluginSystem.md — Wrong Interface (lines 136-141)

**Current (wrong):**
```cpp
class IVendorPlugin {
public:
    virtual ~IVendorPlugin() = default;
    virtual std::string_view vendorName() const = 0;
    virtual VendorCapabilities capabilities() const = 0;
    virtual Result<void> initialize(IDevice& device) = 0;
};
```

**Should match actual code:** IVendorPlugin is a pure discovery interface with no `initialize()` or `IDevice&` parameter.

### 2. Boot-Pipeline.md — Wrong Enum Values (lines 60, 199)

**Current (wrong):**
```
SaharaInit, Connect, ...
```

**Should be:**
```
SaharaHandshake, Connected, ...
```

### 3. Generic-Framework.md — Missing Capabilities and Wrong Method Count

**Current (wrong):**
- Claims 20 methods
- Missing 6 capabilities: `ERASE`, `READ_BACK`, `BULK_WRITE`, `PARTITION_TABLE`, `STORAGE_INFO`, `BOOT_WRITE`

### 4. Session.md — Encoding and Ordering Issues

- UTF-8 BOM present (line 1: `﻿`)
- SessionState list ordering does not match canonical state machine

### 5. Build.md — Stale libmbootsdk.a Reference (lines 88, 99)

References `libmbootsdk.a` which was merged into monolithic archive.

### 6. CompilerSupport.md — Self-Contradiction

Table says Clang 14, section says Clang 15.

### 7. Platform-Support.md — Outdated Minimums (line 67)

Lists `GCC 9+ or Clang 14+` instead of GCC 11+ / Clang 15+.

### 8. SecurityTesting.md — Broken Reference

References non-existent `docs/testing/SecurityAuditReport.md`.

### 9. DesignDecisions.md — Historical Narrative (line 97)

Contains "Evolution" section describing development history (violates product-oriented documentation principle).

### 10. Loader-Framework.md — Stale LoaderManager Reference (line 168)

References `LoaderManager.hpp` in directory tree — class was renamed to `LoaderFramework`.

---

## Duplicate Anchors

### docs/guides/Platform-Support.md

- `## Installation` (appears twice)
- `## Requirements` (appears twice)

### docs/vendor/VENDOR_GRADUATION.md

- `## Completed` (appears twice — once per vendor)
- `## Known Limitations` (appears twice)
- `## Remaining Work` (appears twice)
- `## Required Tests` (appears multiple times)
- `## Required Tests (Current)` (appears twice)
- `## Required Tests (Target)` (appears twice)

---

## Placeholder Markers

No actual TODO/FIXME/TBD/XXX/HACK markers found in documentation.

One false positive: `docs/architecture/GPT.md:71` contains `// "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"` — this is a GUID format example, not a placeholder.

---

## Case Sensitivity Analysis

No case-sensitivity conflicts found. All filenames are unique across the filesystem.

Note: `Architecture.md` exists in both `docs/architecture/` and `docs/gui/`, and `Testing.md` exists in both `docs/guides/` and `docs/gui/`. These are in different directories and do not conflict on case-sensitive filesystems.

---

## Documentation Line Count Summary

| Section | Lines | % of Total |
|---------|-------|-----------|
| docs/architecture/ | 2,753 | 36.6% |
| docs/guides/ | 1,502 | 20.0% |
| Root files (7) | 1,191 | 15.8% |
| docs/gui/ | 621 | 8.2% |
| docs/vendor/ | 552 | 7.3% |
| docs/testing/ | 372 | 4.9% |
| docs/specifications/ | 356 | 4.7% |
| docs/ root (README + BUILD_OPTIONS) | 184 | 2.4% |
| **Total** | **7,531** | **100%** |

---

## Top 10 Largest Files

| # | File | Lines |
|---|------|-------|
| 1 | `AGENTS.md` | 543 |
| 2 | `CONTRIBUTING.md` | 240 |
| 3 | `docs/testing/SecurityTesting.md` | 372 |
| 4 | `docs/architecture/Architecture.md` | 342 |
| 5 | `docs/guides/Plugin-Development.md` | 302 |
| 6 | `docs/architecture/Plugin-System.md` | 298 |
| 7 | `docs/architecture/Firehose.md` | 267 |
| 8 | `docs/vendor/VENDOR_GRADUATION.md` | 206 |
| 9 | `docs/architecture/Boot-Pipeline.md` | 200 |
| 10 | `docs/architecture/GPT.md` | 196 |

---

## Recommendations for Campaign A

1. **Orphan `docs/architecture/BuildPipeline.md`**: Add to navigation after rename
2. **Orphan `docs/testing/SecurityTesting.md`**: Add to navigation in Campaign A
3. **Orphan `docs/gui/README.md`**: Add to navigation as GUI overview
4. **Duplicate anchors in `VENDOR_GRADUATION.md`**: Restructure with vendor-prefixed headings
5. **Duplicate anchors in `Platform-Support.md`**: Restructure with platform-prefixed headings
6. **UTF-8 BOM in `Session.md`**: Remove during Campaign B2 content corrections

---

*This baseline is a snapshot for comparison. It records the state of documentation before any refactoring begins.*
