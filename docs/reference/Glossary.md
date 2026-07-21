# Glossary

Single source of truth for terminology used across MBootCore documentation.

| Term | Definition |
|------|------------|
| **BootROM** | Read-only memory in a device that contains the initial boot code. BootROM protocols operate at this level. |
| **Sahara** | Qualcomm's binary protocol for device communication during boot mode. Used for emergency download and debugging. |
| **Firehose** | Qualcomm's XML-based protocol for programming device flash memory. Runs after Sahara handshake. |
| **BROM** | Boot ROM — the first code that executes on a device. MediaTek's BROM protocol is a reference scaffold. |
| **EDL** | Emergency Download Mode — Qualcomm device state that accepts Sahara protocol connections. |
| **GPT** | GUID Partition Table — the partition table format used by target devices. |
| **ELF** | Executable and Linkable Format — binary format for firmware images and programmers. |
| **Programmer** | A small binary loaded onto the device to perform flash operations (e.g., Firehose programmer). |
| **Firmware Package** | A `.pkg` file containing firmware images, manifest, and metadata for flashing. |
| **Session** | An active connection to a device, managing protocol state and transport. |
| **Transport** | The communication channel (USB, Serial, TCP) between host and device. |
| **Vendor** | A device manufacturer (Qualcomm, MediaTek, UNISOC, etc.) with protocol-specific logic. |
| **Plugin** | An extension module implementing `IVendorPlugin` or `IProtocolPlugin` interfaces. |
| **Workflow** | A sequence of operations (flash, verify, backup) executed on a device. |
| **Job** | A single unit of work in the job queue (flash, backup, restore). |
| **Pipeline** | A processing pipeline that transforms data through stages. |
| **Scaffold** | A reference implementation demonstrating extensibility. Not production ready. |
| **Golden Vector** | A validated device configuration used as a reference for testing. |
| **Mbed TLS** | The cryptographic library used by MBootCore (also written as "MbedTLS" in code). |
| **WinUSB** | Windows USB driver used for direct device communication. |
| **Zadig** | A Windows tool for installing WinUSB drivers. |
| **Result\<T\>** | MBootCore's error-handling type — a value-or-error with `isOk()`, `value()`, `error()`. |
| **Clean Architecture** | The architectural pattern where dependencies point inward toward the domain layer. |
| **IVendorPlugin** | The interface that vendor plugins must implement (defined in `docs/sdk/`). |
| **IProtocolPlugin** | The interface that protocol plugins must implement. |
