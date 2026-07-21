# Project Overview

This document helps new contributors understand MBootCore and find their way
around the project.

## What Is MBootCore?

MBootCore is a C++17 framework for BootROM protocols. It provides tools for
discovering, connecting to, and programming devices through low-level boot ROM
protocols such as Qualcomm Sahara and Firehose.

**Production support:** Qualcomm Sahara (binary), Qualcomm Firehose (XML)  
**Reference scaffolds:** MediaTek BROM, UNISOC FDL (extensibility demos, not production ready)

## Why This Architecture?

MBootCore follows Clean Architecture with strict dependency layering:

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

Each layer depends only on the layer below it. This enables:
- Protocol implementations to be swapped without changing the application layer
- Vendor-specific logic to be added via plugins without modifying core code
- Testing at every layer with mock transports and virtual devices

## Where to Start

1. **[Quick Start](../getting-started/QuickStart.md)** — build and run in 5 minutes
2. **[Architecture](../architecture/Overview.md)** — understand the system design
3. **[Contributing](../developer/Contributing.md)** — how to contribute
4. **[Coding Standards](../developer/CodingStandards.md)** — coding rules (see also [AGENTS.md §6](../internal/AGENTS.md))

## Key Documents

| Document | Purpose |
|----------|---------|
| [Architecture](../architecture/Overview.md) | System design and layer model |
| [AGENTS.md](../internal/AGENTS.md) | Project constitution and coding standards |
| [Contributing](../developer/Contributing.md) | Contribution guidelines |
| [Build Guide](../build/Build.md) | How to build |
| [Change Log](ChangeLog.md) | Release history |
