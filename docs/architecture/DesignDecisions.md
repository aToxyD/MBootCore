# Design Decisions

## Result<T> Over Exceptions

All fallible operations return `Result<T>` — a discriminated union of `T` or `ErrorCode`.

**Rationale**: Embedded/boot ROM protocols are inherently unreliable (voltage drops, cable disconnects, device crashes). Exceptions provide poor control flow for these scenarios — every I/O operation would need a try-catch. `Result<T>` makes errors explicit and forces callers to handle them.

**Exception to the rule**: `noexcept` functions that cannot fail (getters, simple conversions) remain `noexcept`. String-to-number conversions (`std::stoul`) are wrapped in try-catch to prevent `noexcept` violations.

## No Global State

Zero singletons, zero global variables, zero static state.

**Rationale**: Multiple sessions may exist simultaneously (multiple devices). Global state prevents this and makes testing impossible.

## Composition Over Inheritance

Protocols compose interfaces (`IProtocol` + `ITransport` + `ILogger`), not inherit behavior.

**Rationale**: Separates concerns — a protocol implementation should not know how transport works, and vice versa. Enables swapping implementations (USB ↔ Serial ↔ TCP) without protocol changes.

## ISP Compliance (No Logger Dependence in IProtocol)

`IProtocol` does NOT have a `setLogger()` method. Logging is injected via constructor.

**Rationale**: Following the Interface Segregation Principle. A protocol implementation receives a logger reference at construction time, not through a setter on the interface. This keeps `IProtocol` focused on protocol operations.

## Firehose Uses XML, Not IPacket

Firehose commands use `FirehoseCommand` base class (not `IPacket`), XML serialization (not binary).

**Rationale**: Firehose is fundamentally an XML-based protocol. Forcing it into `IPacket` would create a leaky abstraction — the binary packet model (command + length + payload) does not fit XML command + optional binary data chunks. The `IProtocol` interface is shared, but the packet model is not.

Implementation: `FirehoseXmlEngine` is a standalone XML parser/serializer with zero protocol or transport knowledge. `FirehoseCommand` typed classes convert to/from `XmlElement` trees. The `FirehoseStreamEngine` handles chunked data transfer separately.

## Setter-Based Injection in Session

`Session` accepts dependencies via setters (`setTransport`, `setProtocol`, `setLogger`), not constructor parameters.

**Rationale**: Most users want defaults (USB transport, Sahara protocol, console logger). Setters enable optional customization without forcing every user to construct all dependencies. This is a pragmatic tradeoff over constructor injection.

## Separate Serializer/Parser per Protocol

Each binary protocol owns its serializer and parser. Sahara has `SaharaPacketSerializer` and `SaharaPacketParser`. Firehose has `FirehoseXmlEngine`.

**Rationale**: Different protocols use different serialization formats (binary vs XML). Centralizing `IPacketSerializer` / `IPacketParser` would force binary-only protocols into a shared abstraction that doesn't fit XML.

## Registry-Based Discovery Over Switch Statements

Protocols register themselves via `ProtocolRegistry`. No `if(vendor == Qualcomm)` or `switch(protocolType)` exists in the generic layer.

**Rationale**: Adding a new protocol requires implementing 3 interfaces + 3 register calls. Zero changes to core infrastructure.

## Scoring-Based Negotiation Over Hard Matching

Negotiators return confidence scores (0–200+). The engine picks the highest.

**Rationale**: Naturally handles ambiguous cases (e.g., Qualcomm device in Firehose mode — both Sahara and Firehose negotiators match, but Firehose scores higher because bootMode matches).

## Move-Only Semantics

ProtocolRegistry owns unique_ptrs (no copying). Engines are move-deleted (reference to registry). DeviceDescriptor is copyable.

**Rationale**: Matches RAII + move semantics pattern. Clear ownership, no accidental copies of complex objects.

## PluginContext Stores Raw Pointer to DeviceManager

Not unique_ptr — because DeviceManager itself requires ProtocolRegistry&, creating circular ownership.

## IProtocolPlugin and IVendorPlugin Separate

Not merged into a single "vendor protocol plugin" because a vendor may provide multiple protocol plugins or a vendor plugin without any protocol.

## PluginConfig Separate from PluginMetadata

Config is mutable at runtime (enable/disable, priority override). Metadata is immutable after creation.

## Dependency Graph Stored in PluginManager

`dependencyGraph_` tracks dependency chains for circular detection without requiring plugins to declare reverse dependencies.

## Package Model in Firmware-Namespace

The firmware package engine is completely isolated from protocol headers (no Sahara, Firehose includes).

## Directory Structure Alignment

The directory structure mirrors the Clean Architecture layer hierarchy: `domain/` → `core/` → `core/protocols/` → `transport/` → `device/` → `loader/` → `generic/` → `elf/` → `gpt/` → `discovery/` → `pipeline/` → `session/` → `job/` → `plugin/` → `firmware/`. Each directory corresponds to one architectural layer.

## Monolithic Static Archive Over Separate Libraries

All third-party dependencies (zlib, Mbed TLS, libusb, SDK) are merged into `libmbootcore.a` via a `POST_BUILD` archive merge step. Consumers link only `MBootCore::mbootcore` — zero `find_package` calls for internal dependencies.

**Rationale**: MBootCore's dependencies are implementation details. Exposing them to consumers creates unnecessary coupling: consumers must locate the correct version of each dependency, must not accidentally link a different version, and must not see third-party headers in the install tree. A monolithic archive eliminates all three concerns.

**Evolution**: The original design used separate library targets (`mbootcore`, `mbootsdk`) with a hand-written `MBootCoreTargets.cmake`. This was replaced by:
1. OBJECT library (`mbootcore_objects`) — all compile properties and PRIVATE deps live here, preventing leakage into `install(EXPORT)`.
2. STATIC wrapper (`mbootcore`) — zero link dependencies, export-clean.
3. `install(EXPORT MBootCoreTargets)` — CMake generates the targets file, replacing the hand-written version.
4. Three hermetic verification gates — prevent regression to the previous state.

**Why not shared library**: The project targets embedded/low-level environments where shared libraries are often unavailable. Static linking provides deterministic binary layout and zero runtime dependency risk.

**Why not separate archives**: Consumer burden is unacceptable. A framework requiring five `find_package` calls before `target_link_libraries` is not production-ready.

The monolithic archive pattern is the result of evaluating multiple alternatives. Separate libraries burden consumers with locating each dependency individually. Shared libraries are unsuitable for embedded and low-level target environments. Header-only inclusion creates maintenance burden. The OBJECT + STATIC wrapper pattern with post-build archive merge provides a single consumer integration point while preserving clean install-tree guarantees.
