# SDK API Reference

## Classes

### VendorSDK

Main builder class for the Vendor SDK.

**Methods:**
- registerVendor(const VendorRegistration&) – Register a vendor
- registerProtocol(const ProtocolRegistration&) – Register a protocol
- registerTransport(const TransportRegistration&) – Register a transport
- registerWorkflow(const WorkflowRegistration&) – Register a workflow
- registerJob(const JobRegistration&) – Register a job
- registerPackage(const PackageRegistration&) – Register a package format
- registerDiscovery(const DiscoveryRegistration&) – Register a discovery module
- registerCapability(const CapabilityRegistration&) – Register a capability
- finalize() – Validate all registrations and generate report
- clear() – Clear all registrations

### PluginManifest

Manages plugin metadata with JSON serialization.

**Methods:**
- toJson() – Serialize to JSON string
- fromJson(string) – Deserialize from JSON string
- isValid() – Check if manifest is valid
- validate() – Get validation errors

### PluginValidator

Validates all registration types.

**Methods:**
- validateVendor() – Validate vendor registration
- validateProtocol() – Validate protocol registration
- validateTransport() – Validate transport registration
- validateAll() – Validate all components at once

### PluginCompatibility

Checks compatibility between components.

**Methods:**
- checkVendorCompatibility() – Check vendor compatibility
- checkProtocolCompatibility() – Check protocol compatibility
- checkTransportCompatibility() – Check transport compatibility
- checkPackageCompatibility() – Check package compatibility
- checkSDKVersion() – Check SDK version compatibility
- checkAll() – Check all compatibility

### SDKInfo

Provides SDK identity information.

**Properties:**
- name() – SDK name ("MBootCore SDK")
- version() – SDK version ("1.0.0")
- apiVersion() – API version
- buildInfo() – Build metadata

### SDKDoctor

Diagnostic tool for SDK issues.

**Methods:**
- runAllChecks() – Run all diagnostic checks
- checkInstallation() – Verify SDK installation
- checkCompiler() – Verify compiler
- checkDependencies() – Verify dependencies
- checkRuntime() – Verify runtime environment
- checkPlugins() – Verify plugin directories
- checkEnvironment() – Verify system environment
- generateReport() – Generate diagnostic report

## SDK Headers

The SDK consists of 19 public headers in sdk/include/sdk/:

| Header | Purpose |
|--------|---------|
| APICompatibility.hpp | API compatibility checks |
| CapabilityRegistration.hpp | Capability registration structures |
| DiscoveryRegistration.hpp | Discovery registration structures |
| JobRegistration.hpp | Job registration structures |
| PackageRegistration.hpp | Package registration structures |
| PluginCompatibility.hpp | Plugin compatibility reporting |
| PluginDependencyGraph.hpp | Plugin dependency management |
| PluginManifest.hpp | Plugin metadata and serialization |
| PluginMetadata.hpp | Extended plugin metadata types |
| PluginValidator.hpp | Registration validation |
| ProtocolRegistration.hpp | Protocol registration structures |
| SDKInfo.hpp | SDK identity and diagnostics (SDKDoctor) |
| SDKValidator.hpp | SDK installation validator |
| TransportRegistration.hpp | Transport registration structures |
| VendorRegistration.hpp | Vendor registration structures |
| VendorSDK.hpp | Main Vendor SDK builder interface |
| VendorSDKFactory.hpp | Factory for creating VendorSDK instances |
| Version.hpp | Semantic versioning structures |
| WorkflowRegistration.hpp | Workflow registration structures |

## Version Format

Versions use semantic versioning (X.Y.Z) with the following rules:
- Must contain only digits and dots
- Must have at least one dot (e.g., "1.0")
- Up to three parts supported (e.g., "1.2.3")
