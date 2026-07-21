# Release Process

This document describes how MBootCore releases are created and verified.

## Version Numbering

MBootCore follows [Semantic Versioning](https://semver.org/):
- **MAJOR:** Incompatible API changes
- **MINOR:** New functionality in a backwards-compatible manner
- **PATCH:** Backwards-compatible bug fixes

The current version is stored in the `VERSION` file at the project root.

## Creating a Release

### Prerequisites

- Clean source tree (no uncommitted changes)
- CMake 3.20+, GCC 11+ or Clang 12+

### Steps

```bash
# 1. Run the release pipeline
./scripts/create-release.sh

# 2. Modes available
./scripts/create-release.sh --mode full      # All components (default)
./scripts/create-release.sh --mode minimal   # Core library only
./scripts/create-release.sh --mode nightly   # Debug with ASan + UBSan
./scripts/create-release.sh --mode coverage  # Debug with code coverage
```

The pipeline runs: configure → build → tests → export validation → SDK archive.

### Options

| Flag | Description |
|------|-------------|
| `--skip-tests` | Skip test suite execution |
| `--skip-export-validation` | Skip export validation tests |
| `--allow-dirty` | Allow uncommitted changes |
| `--verbose` | Show all commands |
| `-j N` | Parallel job count |

## Verifying a Release

```bash
# Verify a package in dist/
./scripts/verify-release.sh

# Verify a specific package directory
./scripts/verify-release.sh --package-dir dist/

# Verify an installed tree
./scripts/verify-release.sh --install-dir /opt/MBootCore
```

Verification checks:
- Package files exist (ZIP, optional DEB)
- Archive contains required directories (include/, lib/, cmake/)
- 200+ public headers and 19+ SDK headers present
- CMake config files are valid (no absolute source paths)
- SHA-256 checksums match
- `find_package(MBootCore)` works in a consumer project
- Consumer executable builds and runs

## Test Classification

Tests are classified as:
- **Product tests:** Core functionality — failures block the release
- **Environment tests:** Backend/transport selection — failures are warnings

## Release Artifacts

| Artifact | Description |
|----------|-------------|
| `dist/MBootCore-<version>-<platform>.zip` | Release archive |
| `dist/SHA256SUMS.txt` | SHA-256 checksums |
| `lib/cmake/MBootCore/` | CMake package config |

## See Also

- [Build Guide](../build/Build.md) — build instructions
- [Platform Support](../build/PlatformSupport.md) — supported platforms
- [Change Log](ChangeLog.md) — release history
