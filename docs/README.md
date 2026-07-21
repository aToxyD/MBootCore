# MBootCore Documentation

MBootCore is a professional C++17 framework for BootROM protocols. It provides a complete toolchain for discovering, connecting to, and programming devices through low-level boot ROM protocols such as Qualcomm Sahara and Firehose.

## Architecture Overview

```
Application Layer        ─── Session, CLI, GUI
    │
Service Layer            ─── Pipeline, Job, Workflow, Plugin
    │
Capability Layer         ─── Discovery, Firmware, GPT, ELF
    │
Protocol Layer           ─── Sahara, Firehose
    │
Transport Layer          ─── USB, Serial, TCP, UDP
    │
Domain Layer             ─── Interfaces, Types, Error Handling
```

## Documentation Map

### Getting Started

| Document | Description |
|----------|-------------|
| [Architecture](architecture/Overview.md) | Full architecture description — start here |
| [Build Guide](build/Build.md) | Build system, CMake options, dependencies |
| [SDK API](sdk/Overview.md) | Public API reference |

### Architecture

| Document | Description |
|----------|-------------|
| [Sahara Protocol](architecture/Sahara.md) | Sahara binary protocol implementation |
| [Firehose Protocol](architecture/Firehose.md) | Firehose XML protocol implementation |
| [Session Engine](architecture/Session.md) | Device session management |
| [Boot Pipeline](architecture/BootPipeline.md) | Boot stage orchestration |
| [Device Discovery](architecture/DeviceDiscovery.md) | Device detection and protocol negotiation |
| [Generic Flash Framework](architecture/GenericFlash.md) | Protocol-agnostic flash abstraction |
| [Loader Framework](architecture/LoaderFramework.md) | Boot programmer management |
| [Firmware Package Engine](architecture/FirmwarePackage.md) | Firmware packaging and execution |
| [Plugin System](architecture/PluginSystem.md) | Extension framework |
| [ELF Engine](architecture/ELF.md) | ELF parsing and validation |
| [GPT Engine](architecture/GPT.md) | GUID Partition Table management |
| [Design Decisions](architecture/DesignDecisions.md) | Architectural rationale |
| [Security Architecture](architecture/Security.md) | Security subsystem design |

### Specifications

| Document | Description |
|----------|-------------|
| [Protocols](specifications/Protocols.md) | Protocol comparison and transport model |
| [Boot Flow](specifications/BootFlow.md) | End-to-end boot sequence |
| [File Formats](specifications/FileFormats.md) | Binary format specifications |

### Build

| Document | Description |
|----------|-------------|
| [Build Guide](build/Build.md) | Build system, CMake options, dependencies |
| [Compiler Support](build/CompilerSupport.md) | Toolchain requirements |
| [Platform Support](build/PlatformSupport.md) | Supported platforms |

### Testing

| Document | Description |
|----------|-------------|
| [Testing](testing/Testing.md) | Test suite organization |
| [Hardware Testing](testing/HardwareTesting.md) | Hardware validation infrastructure |

### User Guide

| Document | Description |
|----------|-------------|
| [Configuration](user-guide/Configuration.md) | Configuration system |
| [Logging](user-guide/Logging.md) | Structured logging |
| [Diagnostics](user-guide/Diagnostics.md) | Profiling, memory tracking, fault injection |

### Developer

| Document | Description |
|----------|-------------|
| [Thread Safety](developer/ThreadSafety.md) | Concurrency model |

### SDK

| Document | Description |
|----------|-------------|
| [Plugin Development](sdk/PluginDevelopment.md) | Creating plugins |
| [SDK API Reference](sdk/Overview.md) | Public API reference |

### Examples

| Example | Description |
|---------|-------------|
| [minimal](../examples/minimal/main.cpp) | Minimal device detection and connection using Session API |
| [sahara](../examples/sahara/main.cpp) | Sahara protocol upload flow |
| [firehose](../examples/firehose/main.cpp) | Firehose protocol programming flow |
| [basic_connect](../examples/basic_connect) | Runtime-based device connection and health monitoring |
| [device_discovery](../examples/device_discovery) | Multi-device discovery and probing |
| [cli_embedding](../examples/cli_embedding) | Embedding the CLI argument parser |
| [plugin_creation](../examples/plugin_creation) | Creating and managing plugins |
| [job_pipeline](../examples/job_pipeline) | Job pipeline and scheduling |
| [flash_partition](../examples/flash_partition) | Partition flashing with progress callbacks |
| [backup_partition](../examples/backup_partition) | Partition backup and restore |
| [runtime_usage](../examples/runtime_usage) | Full Runtime API demonstration |
| [workflow_execution](../examples/workflow_execution) | Workflow builder and execution |
| [custom_vendor](../examples/custom_vendor) | Custom vendor registration |
| [sdk](../examples/sdk/) | SDK plugin examples (vendor, protocol, workflow, package, discovery) |
| [dynamic](../examples/dynamic/) | Dynamic plugin loading examples |

### GUI (MBoot Studio)

| Document | Description |
|----------|-------------|
| [Architecture](gui/Architecture.md) | GUI layer structure |
| [Components](gui/Components.md) | Widget catalog |
| [Models](gui/Models.md) | Data models |
| [Testing](gui/Testing.md) | GUI test methodology |
| [Theme](gui/Theme.md) | Theme system |
| [Shortcuts](gui/Shortcuts.md) | Keyboard shortcuts |

### Vendor Documentation

| Document | Description |
|----------|-------------|
| [Vendor Maturity](vendor/MaturityModel.md) | Formal maturity model with four states and exit criteria |
| [Vendor Graduation](vendor/Graduation.md) | Per-vendor graduation checklists |
| [Hardware Validation](vendor/HardwareValidation.md) | Hardware testing framework |
| [Golden Vector Policy](vendor/GoldenVectorPolicy.md) | Golden vector creation and maintenance rules |
| [Scaffold Philosophy](vendor/ScaffoldPhilosophy.md) | Why MediaTek and UNISOC remain Scaffold |

## Recommended Reading Order

1. [Architecture](architecture/Overview.md) — understand the system
2. [Design Decisions](architecture/DesignDecisions.md) — understand the philosophy
3. [Sahara Protocol](architecture/Sahara.md) or [Firehose Protocol](architecture/Firehose.md) — protocol details
4. [Boot Flow](specifications/BootFlow.md) — how it all connects
5. [Plugin System](architecture/PluginSystem.md) — how to extend
6. [Build Guide](build/Build.md) — how to build
