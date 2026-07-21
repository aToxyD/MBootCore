# Firmware Package Engine

## Overview

The Firmware Package Engine provides a complete subsystem for describing, loading, validating, resolving, and executing firmware packages across any supported vendor. It is completely protocol-agnostic — no Sahara, Firehose, or any protocol headers are included.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    FirmwareExecutor                          │
│  Bridges FirmwarePackage → JobPipeline → Device operations   │
├─────────────────────────────────────────────────────────────┤
│                    FlashPlanGenerator                        │
│  Generates ordered flash steps from package + device info    │
├─────────────────────────────────────────────────────────────┤
│                   FirmwareResolver                           │
│  Matches package vendor/protocol against device descriptor   │
├─────────────────────────────────────────────────────────────┤
│                   FirmwareValidator                          │
│  Validates manifest, dependencies, integrity, compatibility  │
├─────────────────────────────────────────────────────────────┤
│                   IFirmwareReader                            │
│  DirectoryFirmwareReader | RawFirmwareReader | ZipFirmwareReader
├─────────────────────────────────────────────────────────────┤
│                   FirmwarePackage                            │
│  PackageMetadata + PackageManifest + FirmwareImage[]         │
├─────────────────────────────────────────────────────────────┤
│                   FirmwareTypes                              │
│  Enums, structs, version, dependency, signature, plan types  │
└─────────────────────────────────────────────────────────────┘
```

## Manifest Format

The directory reader expects `manifest.json` in the package directory:

```json
{
    "vendor": "Qualcomm",
    "platform": "SM8450",
    "chipset": "SM8450",
    "protocol": "sahara",
    "version": "1.0.0",
    "build_date": "2026-06-30",
    "author": "Vendor",
    "images": [
        {"name": "prog", "type": "Programmer", "partition": "prog", "file": "prog.mbn"},
        {"name": "gpt", "type": "GPT", "partition": "gpt_primary", "file": "gpt.bin"},
        {"name": "boot", "type": "Boot", "partition": "boot", "file": "boot.img"},
        {"name": "system", "type": "System", "partition": "system", "file": "system.img"}
    ]
}
```

## Components

### FirmwareTypes

Core type definitions:

- `PackageMetadata` — vendor, platform, chipset, protocol, version, build date
- `PackageManifest` — ordered list of firmware images with metadata
- `FirmwareImage` — name, type, partition target, data, size, hash
- `FirmwareVersion` — semantic version with comparison operators
- `PackageDependency` — name and version range for inter-package dependencies
- `ValidationError` — error context with field and reason for precise reporting
- `FlashStep` — individual flash operation (program, erase, verify, GPT update)

### FirmwarePackage

Move-only package model holding metadata, manifest, and images:

```cpp
class FirmwarePackage {
    const PackageMetadata& metadata() const;
    const PackageManifest& manifest() const;
    const std::vector<FirmwareImage>& images() const;

    void addImage(FirmwareImage image);
    std::optional<FirmwareImage> findImage(std::string_view name) const;
    std::optional<FirmwareImage> findImageByType(ImageType type) const;

    // Validation
    bool isValid() const;
    std::vector<ValidationError> validate() const;
};
```

### FirmwareReaders

```cpp
class IFirmwareReader {
    Result<std::unique_ptr<FirmwarePackage>> read(std::string_view path);
    bool canRead(std::string_view path) const;
    std::string name() const;
};
```

Implementations:
- **DirectoryFirmwareReader** — reads `manifest.json` from a directory, loads image files referenced in the manifest
- **RawFirmwareReader** — reads a single raw binary file as a single-image package
- **ZipFirmwareReader** — reads firmware from ZIP archives

### FirmwareValidator

Multi-level validation:

```cpp
class FirmwareValidator {
    // Manifest validation
    Result<Unit> validateManifest(const PackageManifest& manifest);

    // Dependency validation
    Result<Unit> validateDependencies(
        const FirmwarePackage& package,
        const std::vector<FirmwarePackage>& available);

    // Integrity validation
    Result<Unit> validateIntegrity(const FirmwarePackage& package);

    // Device compatibility
    Result<Unit> validateDeviceCompatibility(
        const FirmwarePackage& package,
        const DeviceDescriptor& device);
};
```

### FirmwareResolver

Matches packages to devices:

```cpp
class FirmwareResolver {
    Result<FirmwarePackage> resolve(
        const std::vector<FirmwarePackage>& available,
        const DeviceDescriptor& device);

    bool isVendorCompatible(const FirmwarePackage& pkg, const DeviceDescriptor& device);
    bool isProtocolCompatible(const FirmwarePackage& pkg, const DeviceDescriptor& device);
};
```

### FlashPlanGenerator

Generates ordered flash operations from a package:

```cpp
class FlashPlanGenerator {
    Result<std::vector<FlashStep>> generate(
        const FirmwarePackage& package,
        const DeviceInfo& deviceInfo);

    Result<std::vector<FlashStep>> generateForPartitions(
        const FirmwarePackage& package,
        const std::vector<std::string>& partitionNames);
};
```

### FirmwareExecutor

Bridges firmware execution to the Job Pipeline:

```cpp
class FirmwareExecutor {
    Result<Unit> execute(
        const std::vector<FlashStep>& plan,
        IFlashDevice& device,
        ProgressCallback progress);

    Result<Unit> cancel();
};
```

Uses existing job types (FlashJob, VerifyJob, EraseJob, GPTUpdateJob) from the Job Engine.

### VirtualFirmware

Virtual test infrastructure:

- `VirtualFirmwareGenerator` — creates firmware packages programmatically
- `VirtualFirmwareReader` — reads virtual packages
- `VirtualFirmwareRepository` — manages a collection of virtual packages
- Supports failure injection for testing validation and resolution logic

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Package model in `firmware/` namespace | Isolation from protocol layer |
| `FirmwarePackage` is move-only | Large data buffers, no accidental copies |
| Manual JSON parser (no dependency) | Avoids requiring nlohmann-json or QJsonDocument |
| XOR hash for integrity | Zero-dependency checksum, extensible to SHA256 |
| `ValidationError` struct with context | Enables precise error reporting to users |
| Executor bridges to Job Engine | Reuses existing FlashJob/VerifyJob/EraseJob/GPTUpdateJob |

## See Also

- [Overview](Overview.md) — architecture layer diagram
- [GPT Partition Manager](GPT.md) — partition table operations
