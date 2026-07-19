# Build Pipeline Architecture

## Goals

- Establish a single source of truth for build configurations
- Eliminate drift between local development and release scripts
- Reduce duplication across CI workflows
- Preserve Modern CMake per-target philosophy for instrumentation
- Keep platform specifics out of build configuration

## Build Architecture

```
Developer
    ↓
Scripts (orchestration)
    ↓
Build Configuration Layer (source of truth)
    ↓
CMake (build system)
    ↓
Build artifacts
    ↓
Packaging
    ↓
Verification
    ↓
CI (release)
```

The pipeline has a unidirectional dependency flow. Each layer consumes the
layer above it. No layer bypasses another.

## Configuration Dimensions

Build configurations are organized along three independent dimensions:

| Dimension | Purpose |
|-----------|---------|
| Build Configuration | Optimization level, debug info, warnings |
| Feature Configuration | What components get built |
| Instrumentation | Runtime analysis tools |

These dimensions are independent. A build configuration selects one option
from each dimension. For example: Debug + Tests enabled + ASan instrumented.

## Layer Responsibilities

| Layer | Responsibility |
|-------|---------------|
| Build Configuration | Canonical build configurations — cache variables, build type, feature flags. Current implementation: CMake Presets. |
| Scripts | Release orchestration — configuration selection, platform detection, packaging invocation. |
| CI (release) | Packaging — invokes release presets. Builds packages. Publishes GitHub Releases. |
| CMake modules | Implementation — flag assembly, target instrumentation, warnings. |

## Architectural Decisions

- Build configurations are defined in a single canonical location. No manual
  flags in scripts or CI.

- Scripts orchestrate builds. They select the appropriate configuration
  internally based on the build scenario.

- CI release workflows consume release presets directly for packaging.
  Verification (tests, sanitizers, static analysis) is performed locally
  before creating a release tag.

- CI release workflows build packages and publish GitHub Releases.

- Instrumentation is applied per-target via explicit function calls, not via
  global compiler flags. This preserves the ability to instrument selected
  targets without affecting the entire build.

- SDKDistribution and CPack coexist. They serve different distribution
  channels: developer SDK archives with provenance metadata versus
  OS-native packages.

- Platform-specific concerns (generator selection, toolchain paths) remain
  outside build configurations. They belong in orchestration or CI.

## Extension Principles

- New build configurations are introduced through the Build Configuration
  layer. No ad-hoc flags in scripts or workflows.

- Platform-specific behavior belongs in orchestration or CI, not in build
  configuration.

- Instrumentation must remain target-based. Global compiler flags are not
  used for sanitizer or analysis instrumentation.

- Release packaging must reuse the same build configuration as local
  releases. No separate release-only configuration paths.

## Non-goals

- No redesign of the CMake build system
- No changes to frozen architectural decisions
- No API/ABI changes
- No library logic changes
- No new features
- No changes to the monolithic archive pattern
