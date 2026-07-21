# Loader Framework

## Overview

The Loader Framework manages boot programmer binaries (loaders) in a protocol- and vendor-agnostic way. It provides search, matching, validation, caching, ELF inspection, and selection — all without hardcoding file paths, vendor logic, or protocol details.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    LoaderFramework                          │
│  (Facade — orchestrates all components)                    │
├──────────┬──────────┬──────────┬──────────┬───────────────┤
│ ILoader  │ ILoader  │ ILoader  │ ILoader  │  IElfInspector │
│ Repository│ Matcher  │ Validator│ Cache   │               │
├──────────┼──────────┼──────────┼──────────┼───────────────┤
│ Loader   │ Loader   │ Loader   │ Loader   │  ElfInspector  │
│ Repository│ Matcher  │ Validator│ Cache   │               │
├──────────┴──────────┴──────────┴──────────┴───────────────┤
│                    ISelectionPolicy                        │
│                    (PrioritySelection)                     │
└─────────────────────────────────────────────────────────────┘
```

Each component implements an interface so users can inject custom implementations.

## Components

### LoaderMetadata

A struct (not an interface) holding all metadata:

| Field | Type | Description |
|-------|------|-------------|
| `vendor` | `std::string` | Vendor name (Qualcomm, MediaTek, ...) |
| `protocol` | `std::string` | Protocol name (Sahara, Firehose, ...) |
| `chipset` | `std::string` | Chipset model (SM8250, MT6785, ...) |
| `msmId` | `uint32_t` | MSM chip ID |
| `oemId` | `uint32_t` | OEM ID |
| `modelId` | `uint32_t` | Model ID |
| `pkhash` | `vector<uint8_t>` | 32-byte PK Hash |
| `version` | `std::string` | Loader version string |
| `build` | `std::string` | Build number |
| `securityVersion` | `uint32_t` | Security version |
| `storageType` | `std::string` | eMMC, UFS, NAND, NOR, SPI |
| `supportedMemory` | `vector<string>` | Memory types (DDR4, LPDDR5, ...) |
| `loaderSize` | `uint64_t` | Size in bytes |
| `hash` | `vector<uint8_t>` | Loader content hash |
| `hashAlgorithm` | `std::string` | Hash algorithm name |
| `isSigned` | `bool` | Whether loader is signed |

### ILoaderRepository

Scans and caches loader files from the filesystem:

```cpp
class ILoaderRepository {
    Result<std::vector<LoaderMetadata>> scanDirectory(std::string_view path);
    Result<ByteBuffer> loadLoader(std::string_view path);
    Result<std::vector<LoaderMetadata>> listCached();
    void clearCache();
};
```

### ILoaderMatcher

Matches loaders against device criteria using scoring:

```cpp
class ILoaderMatcher {
    Result<std::vector<ScoredMatch>> findMatches(
        const ILoaderRepository& repository,
        const DeviceId& deviceId);

    ScoredMatch bestMatch(
        const ILoaderRepository& repository,
        const DeviceId& deviceId);
};
```

Matching uses descending priority scoring:

| Criterion | Score | Description |
|-----------|-------|-------------|
| PK Hash | 80 | Exact match on 32-byte public key hash |
| MSM ID | 50 | Chip ID match (msmId) |
| Vendor | 40 | Vendor name match |
| Chipset | 30 | Chipset model match |
| Protocol | 15 | Protocol name match |
| Storage | 10 | Storage type match |

### ILoaderValidator

Validates loader integrity and compatibility:

```cpp
class ILoaderValidator {
    Result<Unit> validateIntegrity(const LoaderMetadata& metadata, const ByteBuffer& data);
    Result<Unit> validateCompatibility(const LoaderMetadata& metadata, const DeviceId& deviceId);
    bool isSupported(std::string_view vendor, std::string_view protocol);
};
```

### ILoaderCache

Caches parsed loaders for reuse:

```cpp
class ILoaderCache {
    void cache(std::string_view key, LoaderMetadata metadata);
    std::optional<LoaderMetadata> find(std::string_view key);
    void remove(std::string_view key);
    void clear();

    // Configuration
    void setMaxEntries(size_t max);
    void setMaxMemory(size_t maxBytes);
};
```

### IElfInspector

Extracts ELF header information from loader binaries:

```cpp
class IElfInspector {
    Result<ElfHeaderInfo> inspect(const ByteBuffer& loaderData);
    Result<bool> isValidElf(const ByteBuffer& loaderData);
    Result<uint32_t> getEntryPoint(const ByteBuffer& loaderData);
    Result<std::vector<ProgramHeaderInfo>> getProgramHeaders(const ByteBuffer& loaderData);
};
```

### LoaderFramework

Facade orchestrating all components:

```cpp
class LoaderFramework {
    LoaderFramework(
        std::unique_ptr<ILoaderRepository> repository,
        std::unique_ptr<ILoaderMatcher> matcher,
        std::unique_ptr<ILoaderValidator> validator,
        std::unique_ptr<ILoaderCache> cache,
        std::unique_ptr<IElfInspector> inspector);

    Result<LoadedLoader> findAndLoad(std::string_view searchPath, const DeviceId& deviceId);
    Result<LoadedLoader> loadFromFile(std::string_view path);
    Result<LoadedLoader> loadFromBuffer(const ByteBuffer& data);

    void clearCache();
};
```

## File Structure

```
lib/include/mbootcore/loader/
├── LoaderMetadata.hpp       — LoaderMetadata struct
├── ILoaderRepository.hpp    — Repository interface
├── ILoaderMatcher.hpp       — Matcher interface
├── ILoaderValidator.hpp     — Validator interface
├── ILoaderCache.hpp         — Cache interface
├── IElfInspector.hpp        — ELF inspector interface
├── ISelectionPolicy.hpp     — Selection policy interface
├── LoaderFramework.hpp      — Facade class
│
├── LoaderManager.hpp        — Manager for loader search paths
└── ProgrammerLoader.hpp     — Concrete loader for boot programmers
```
