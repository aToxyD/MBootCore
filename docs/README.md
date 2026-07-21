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
| [Installation](getting-started/Installation.md) | Platform-specific setup instructions |
| [Quick Start](getting-started/QuickStart.md) | Get running in 5 minutes |
| [Architecture](architecture/Overview.md) | Full architecture description |
| [Build Guide](build/Build.md) | Build system, CMake options, dependencies |
| [SDK API](sdk/Overview.md) | Public API reference |

### Architecture

| Document | Description |
|----------|-------------|
| [Sahara Protocol](architecture/Sahara.md) | Sahara binary protocol implementation |
| [Firehose Protocol](architecture/Firehose.md) | Firehose XML protocol implementation |
| [Session](architecture/Session.md) | Device session management |
| [Boot Pipeline](architecture/BootPipeline.md) | Boot stage orchestration |
| [Device Discovery](architecture/DeviceDiscovery.md) | Device detection and protocol negotiation |
| [Generic Flash](architecture/GenericFlash.md) | Protocol-agnostic flash abstraction |
| [Loader Framework](architecture/LoaderFramework.md) | Boot programmer management |
| [Firmware Package](architecture/FirmwarePackage.md) | Firmware packaging and execution |
| [Plugin System](architecture/PluginSystem.md) | Extension framework |
| [ELF Engine](architecture/ELF.md) | ELF parsing and validation |
| [GPT Engine](architecture/GPT.md) | GUID Partition Table management |
| [Design Decisions](architecture/DesignDecisions.md) | Architectural rationale |
| [Security Architecture](architecture/Security.md) | Security subsystem design |
| [Build Pipeline](architecture/BuildPipeline.md) | Build system architecture |

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
| [Build Options](build/BuildOptions.md) | Complete CMake option reference |
| [Compiler Support](build/CompilerSupport.md) | Toolchain requirements |
| [Platform Support](build/PlatformSupport.md) | Supported platforms |

### Testing

| Document | Description |
|----------|-------------|
| [Testing](testing/Testing.md) | Test suite organization |
| [Security Testing](testing/SecurityTesting.md) | Security test methodology and results |
| [Hardware Testing](testing/HardwareTesting.md) | Hardware validation infrastructure |

### User Guide

| Document | Description |
|----------|-------------|
| [CLI Guide](user-guide/CLI.md) | Command-line tool usage and workflows |
| [MBoot Studio](user-guide/Studio.md) | GUI application guide |
| [Configuration](user-guide/Configuration.md) | Configuration system |
| [Logging](user-guide/Logging.md) | Structured logging |
| [Diagnostics](user-guide/Diagnostics.md) | Profiling, memory tracking, fault injection |

### Developer

| Document | Description |
|----------|-------------|
| [Contributing](developer/Contributing.md) | Contribution guidelines |
| [Thread Safety](developer/ThreadSafety.md) | Concurrency model |

### SDK

| Document | Description |
|----------|-------------|
| [Plugin Development](sdk/PluginDevelopment.md) | Creating plugins |
| [SDK API Reference](sdk/Overview.md) | Public API reference |

### GUI (MBoot Studio)

| Document | Description |
|----------|-------------|
| [Overview](gui/README.md) | GUI overview and features |
| [Architecture](gui/Architecture.md) | GUI layer structure |
| [Components](gui/Components.md) | Widget catalog |
| [Models](gui/Models.md) | Data models |
| [Testing](gui/Testing.md) | GUI test methodology |
| [Theme](gui/Theme.md) | Theme system |
| [Shortcuts](gui/Shortcuts.md) | Keyboard shortcuts |

### Vendor Documentation

| Document | Description |
|----------|-------------|
| [Maturity Model](vendor/MaturityModel.md) | Formal maturity model with four states and exit criteria |
| [Graduation](vendor/Graduation.md) | Per-vendor graduation checklists |
| [Hardware Validation](vendor/HardwareValidation.md) | Hardware testing framework |
| [Golden Vector Policy](vendor/GoldenVectorPolicy.md) | Golden vector creation and maintenance rules |
| [Scaffold Philosophy](vendor/ScaffoldPhilosophy.md) | Why MediaTek and UNISOC remain Scaffold |

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

### Project

| Document | Description |
|----------|-------------|
| [Change Log](project/ChangeLog.md) | Release history |

### Reference

| Document | Description |
|----------|-------------|
| [Third-Party Licenses](reference/ThirdPartyLicenses.md) | Dependency license notices |

### Internal

| Document | Description |
|----------|-------------|
| [AGENTS.md](internal/AGENTS.md) | Project constitution, coding standards, review criteria |

## Recommended Reading Order

1. [Architecture](architecture/Overview.md) — understand the system
2. [Design Decisions](architecture/DesignDecisions.md) — understand the philosophy
3. [Sahara Protocol](architecture/Sahara.md) or [Firehose Protocol](architecture/Firehose.md) — protocol details
4. [Boot Flow](specifications/BootFlow.md) — how it all connects
5. [Plugin System](architecture/PluginSystem.md) — how to extend
6. [Build Guide](build/Build.md) — how to build
