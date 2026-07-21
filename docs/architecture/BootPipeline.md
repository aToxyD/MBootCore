# Boot Pipeline

## Overview

The Boot Pipeline Orchestrator unifies all boot stages into a single coordinated flow. It connects Sahara handshake → Loader Selection → ELF Parsing → Programmer Upload → Programmer Execute → Firehose Detection → Firehose Configuration → Ready state — without any protocol knowing about any other protocol.

The pipeline **coordinates, not implements**. All protocol-specific logic stays in the protocol layer.

## Architecture

```
BootPipeline
  │
  ├── BootContext       — shared state across all stages
  ├── RecoveryStrategy  — configurable failure recovery
  ├── BootPipelineConfig — timeouts, retries, flags
  ├── Stage handlers    — std::function set by caller
  └── ProgressCallback  — reported on every stage change
```

### Key Design Decision: No Protocol Headers in Pipeline

The pipeline header includes zero protocol headers:
- No `#include` of Sahara headers
- No `#include` of Firehose headers
- No `#include` of ELF headers
- Only domain types (DeviceInfo, ErrorCode, etc.)

## Components

### Stage Handlers

Stages are implemented as `std::function` callbacks set by the caller:

```cpp
enum class PipelineStage : uint8_t {
    Disconnected = 0,
    Connected,
    SaharaHandshake,
    VersionNegotiation,
    DeviceDiscovery,
    LoaderSelection,
    ElfParsing,
    MemoryImageBuild,
    ProgrammerUpload,
    ProgrammerExecute,
    FirehoseDetection,
    FirehoseConfiguration,
    Ready,
    Error,
    Cancelled
};

using StageHandler = std::function<Result<Unit>(BootContext& context)>;
```

Each handler receives a `BootPipelineContext` reference containing the shared state and uses it to perform its stage-specific work:

```cpp
pipeline.setHandler(Stage::SaharaInit, [](auto& ctx) -> Result<Unit> {
    auto protocol = ctx.getProtocol<SaharaProtocol>();
    return protocol->handshake();
});
```

### BootContext

Shared mutable state passed across all stages:

```cpp
struct BootContext {
    DeviceId deviceId;
    DeviceInfo saharaDeviceInfo;
    GenericDeviceInfo genericDeviceInfo;
    ProtocolVersion negotiatedVersion;

    LoaderMetadata loaderMetadata;
    elf::MemoryImage memoryImage;

    // Flash config fields...

    ProgressInfo progress;

    PipelineStage currentStage{PipelineStage::Disconnected};
    PipelineStage previousStage{PipelineStage::Disconnected};
    ErrorCode lastError{ErrorCode::Success};
    int stageRetryCount{0};

    std::unordered_map<std::string, std::string> properties;

    ITransport* transport{nullptr};
    ILogger* logger{nullptr};
};
```

Move-only semantics (contains `unique_ptr`).

### BootPipelineConfig

Configuration for pipeline behavior:

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `stageTimeouts` | `std::vector<int>` | Per-stage | Timeout per stage in ms |
| `maxRetries` | `int` | 3 | Maximum retry attempts |
| `retryDelay` | `int` | 500 | Delay between retries in ms |
| `enableRecovery` | `bool` | true | Enable recovery strategies |
| `progressCallback` | `ProgressCallback` | nullptr | Stage completion callback |

### RecoveryStrategy

Per-stage failure recovery rules:

| Rule | Action | Description |
|------|--------|-------------|
| Retry | RetryAction | Re-attempt current stage (up to N times) |
| Rollback | RollbackAction | Return to a previous stage |
| Abort | AbortAction | Immediately fail the pipeline |

```cpp
struct RecoveryStrategy {
    int maxRetries = 3;
    int retryDelayMs = 500;
    std::optional<Stage> rollbackTarget;
    RecoveryAction action = RecoveryAction::Retry;
};
```

## Stage Sequence

```
Stage:  Connected
    Establish transport connection with device

Stage:  SaharaHandshake
    Perform Sahara handshake

Stage:  VersionNegotiation
    Negotiate protocol version

Stage:  DeviceDiscovery
    Discover device identity and capabilities

Stage:  LoaderSelection
    Find matching boot programmer using Loader Framework

Stage:  ElfParsing
    Parse and validate programmer binary

Stage:  MemoryImageBuild
    Build memory image from ELF segments

Stage:  ProgrammerUpload
    Transfer programmer to device via Sahara chunks

Stage:  ProgrammerExecute
    Reset device to run uploaded programmer

Stage:  FirehoseDetection
    Confirm device is in Firehose mode

Stage:  FirehoseConfiguration
    Send configure command with memory settings

Stage:  Ready
    Device ready for flash operations
```

## Pipeline Flow

```
run()
  │
  ├── for each stage (0..N):
  │     ├── execute stage handler
  │     ├── on success: advance to next stage
  │     ├── on failure:
  │     │     ├── apply RecoveryStrategy
  │     │     ├── retry/rollback/abort
  │     │     └── report progress
  │     └── report stage completion
  │
  └── pipeline finished: Ready or Error
```

## Factory

`BootPipelineFactory` creates configured pipelines:

```cpp
// Default pipeline with standard stage handlers
auto pipeline = BootPipelineFactory::createWithDefaults();

// Pipeline from a device descriptor
auto pipeline = BootPipelineFactory::createFromDescriptor(descriptor);

// Custom pipeline
auto pipeline = std::make_unique<BootPipeline>(config);
pipeline->setHandler(Stage::Connect, myConnectHandler);
```

## See Also

- [Overview](Overview.md) — architecture layer diagram
- [Plugin System](PluginSystem.md) — extension points and stage handlers
