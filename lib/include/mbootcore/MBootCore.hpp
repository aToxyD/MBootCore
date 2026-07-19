#pragma once

// Domain Interfaces & Types
#include "domain/Types.hpp"
#include "domain/Error.hpp"
#include "domain/TransportTypes.hpp"
#include "domain/ILogger.hpp"
#include "domain/ITransport.hpp"
#include "domain/IPacket.hpp"
#include "domain/IPacketSerializer.hpp"
#include "domain/IPacketParser.hpp"
#include "domain/IStateMachine.hpp"
#include "domain/IProtocol.hpp"
#include "domain/IDevice.hpp"
#include "domain/ILoader.hpp"
#include "domain/DeviceTypes.hpp"

// Core State Machine
#include "core/state/StateMachine.hpp"

// Sahara Protocol
#include "core/protocols/sahara/SaharaPackets.hpp"
#include "core/protocols/sahara/SaharaProtocol.hpp"
#include "core/protocols/sahara/SaharaStateMachine.hpp"
#include "core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "core/protocols/sahara/SaharaPacketParser.hpp"

// Firehose Protocol
#include "core/protocols/firehose/FirehoseXmlEngine.hpp"
#include "core/protocols/firehose/FirehosePackets.hpp"
#include "core/protocols/firehose/FirehoseStreamEngine.hpp"
#include "core/protocols/firehose/FirehoseProtocol.hpp"
#include "core/protocols/firehose/FlashContext.hpp"
#include "core/protocols/firehose/ChunkEngine.hpp"

// Protocol Platform
#include "protocol/ProtocolTypes.hpp"

// Transport Layer
#include "transport/UsbTransport.hpp"
#include "transport/SerialTransport.hpp"
#include "transport/TcpTransport.hpp"
#include "transport/UdpTransport.hpp"
#include "transport/TransportFactory.hpp"
#include "transport/TransportManager.hpp"
#include "transport/VirtualTransports.hpp"
#include "network/NetworkAddress.hpp"

// Device
#include "device/DefaultDevice.hpp"

// Logging
#include "logging/ConsoleLogger.hpp"
#include "logging/FileLogger.hpp"
#include "logging/NullLogger.hpp"
#include "logging/LoggingTypes.hpp"
#include "logging/StructuredLogger.hpp"

// Loader Framework
#include "loader/LoaderManager.hpp"
#include "loader/ProgrammerLoader.hpp"
#include "loader/LoaderMetadata.hpp"
#include "loader/ILoaderData.hpp"
#include "loader/ILoaderRepository.hpp"
#include "loader/ILoaderMatcher.hpp"
#include "loader/ILoaderValidator.hpp"
#include "loader/ILoaderCache.hpp"
#include "loader/IELFInspector.hpp"
#include "loader/ISelectionPolicy.hpp"
#include "loader/LoaderRepository.hpp"
#include "loader/LoaderMatcher.hpp"
#include "loader/LoaderValidator.hpp"
#include "loader/LoaderCache.hpp"
#include "loader/ElfInspector.hpp"
#include "loader/PrioritySelection.hpp"
#include "loader/LoaderData.hpp"
#include "loader/LoaderFramework.hpp"

// Application Session
#include "application/Session.hpp"

// ELF Engine
#include "mbootcore/elf/ElfTypes.hpp"
#include "mbootcore/elf/ElfModels.hpp"
#include "mbootcore/elf/ElfParser.hpp"
#include "mbootcore/elf/ElfValidator.hpp"
#include "mbootcore/elf/MemoryImageBuilder.hpp"
#include "mbootcore/elf/IProgrammerExecutor.hpp"
#include "mbootcore/elf/VirtualProgrammer.hpp"

// Generic Flash Abstraction Layer
#include "generic/FlashCapability.hpp"
#include "generic/DeviceInfo.hpp"
#include "generic/StorageInfo.hpp"
#include "generic/PartitionModel.hpp"
#include "generic/ProgressInfo.hpp"
#include "generic/IFlashOperation.hpp"
#include "generic/IFlashDevice.hpp"
#include "generic/OperationPipeline.hpp"
#include "generic/ErrorMapping.hpp"
#include "generic/adapter/SaharaAdapter.hpp"
#include "generic/adapter/FirehoseAdapter.hpp"

// Boot Pipeline Orchestrator
#include "pipeline/PipelineStage.hpp"
#include "pipeline/BootContext.hpp"
#include "pipeline/BootPipelineConfig.hpp"
#include "pipeline/RecoveryStrategy.hpp"
#include "pipeline/BootPipeline.hpp"
#include "pipeline/BootPipelineFactory.hpp"

// GPT & Partition Engine
#include "gpt/Guid.hpp"
#include "gpt/GPTModels.hpp"
#include "gpt/GPTParser.hpp"
#include "gpt/GPTWriter.hpp"
#include "gpt/PartitionManager.hpp"
#include "gpt/IImageReader.hpp"
#include "gpt/ImageReaders.hpp"

// Device Discovery & Protocol Negotiation
#include "discovery/DiscoveryTypes.hpp"
#include "discovery/IDeviceDetector.hpp"
#include "discovery/IProtocolNegotiator.hpp"
#include "discovery/IProtocolFactory.hpp"
#include "discovery/ProtocolRegistry.hpp"
#include "discovery/DeviceDiscoveryEngine.hpp"
#include "discovery/ProtocolNegotiationEngine.hpp"
#include "discovery/VirtualDeviceDetector.hpp"

// Device Session & Multi-Device Manager
#include "session/SessionTypes.hpp"
#include "session/ISessionObserver.hpp"
#include "session/SessionLogger.hpp"
#include "session/DeviceSession.hpp"
#include "session/DeviceSessionFactory.hpp"
#include "session/DeviceManager.hpp"

// Job Engine
#include "job/JobTypes.hpp"
#include "job/IJob.hpp"
#include "job/GenericJobs.hpp"
#include "job/JobPipeline.hpp"
#include "job/JobScheduler.hpp"
#include "job/JobHistory.hpp"
#include "job/ProgressAggregator.hpp"
#include "job/RecoveryPolicies.hpp"
#include "job/VirtualJobDevice.hpp"

// Plugin System
#include "plugin/PluginTypes.hpp"
#include "plugin/PluginABI.hpp"
#include "plugin/IPlugin.hpp"
#include "plugin/IProtocolPlugin.hpp"
#include "plugin/PluginContext.hpp"
#include "plugin/PluginManager.hpp"

// Firmware Package Management
#include "firmware/FirmwareTypes.hpp"
#include "firmware/IFirmwareReader.hpp"
#include "firmware/FirmwarePackage.hpp"
#include "firmware/FirmwareReaders.hpp"
#include "firmware/FirmwareValidator.hpp"
#include "firmware/FirmwareResolver.hpp"
#include "firmware/ImageEngine.hpp"
#include "firmware/FlashPlan.hpp"
#include "firmware/FirmwareExecutor.hpp"
#include "firmware/VirtualFirmware.hpp"

// Workflow Engine
#include "workflow/IWorkflow.hpp"
#include "workflow/IWorkflowStep.hpp"
#include "workflow/WorkflowTypes.hpp"
#include "workflow/WorkflowBuilder.hpp"
#include "workflow/WorkflowEngine.hpp"
#include "workflow/WorkflowExecutor.hpp"
#include "workflow/WorkflowFactory.hpp"
#include "workflow/WorkflowHistory.hpp"
#include "workflow/WorkflowProgressEngine.hpp"
#include "workflow/WorkflowRecovery.hpp"
#include "workflow/VirtualWorkflowRuntime.hpp"

// Vendor Framework
#include "vendor/VendorTypes.hpp"
#include "vendor/IVendor.hpp"
#include "vendor/IVendorPlugin.hpp"
#include "vendor/VendorContext.hpp"
#include "vendor/VendorEvents.hpp"
#include "vendor/VendorFactory.hpp"
#include "vendor/VendorMonitor.hpp"
#include "vendor/VendorPackageResolver.hpp"
#include "vendor/VendorPipeline.hpp"
#include "vendor/VendorRegistry.hpp"
#include "vendor/VendorRuntime.hpp"
#include "vendor/VendorSession.hpp"
#include "vendor/CapabilityResolver.hpp"

// Runtime
#include "runtime/IRuntime.hpp"
#include "runtime/IDeviceService.hpp"
#include "runtime/IFirmwareService.hpp"
#include "runtime/IWorkflowService.hpp"
#include "runtime/IPluginService.hpp"
#include "runtime/IDiagnosticsService.hpp"
#include "runtime/Services.hpp"
#include "runtime/RuntimeConfig.hpp"
#include "runtime/RuntimeCallbacks.hpp"
#include "runtime/RuntimeEvents.hpp"
#include "runtime/RuntimeObserver.hpp"
#include "runtime/RuntimeStatistics.hpp"
#include "runtime/RuntimeHardware.hpp"
#include "runtime/RuntimeFactory.hpp"
#include "runtime/RuntimeBuilder.hpp"
#include "runtime/Runtime.hpp"

// Benchmark
#include "benchmark/BenchmarkTypes.hpp"
#include "benchmark/BenchmarkRunner.hpp"

// Configuration
#include "config/ConfigTypes.hpp"
#include "config/ConfigManager.hpp"

// Diagnostics
// Diagnostics
#include "diagnostics/DiagnosticsTypes.hpp"
#include "diagnostics/DiagnosticCheck.hpp"
#include "diagnostics/DiagnosticSession.hpp"
#include "diagnostics/DiagnosticsManager.hpp"

// Security
// Security
#include "security/SecurityTypes.hpp"
#include "security/PermissionSet.hpp"
#include "security/InMemoryStorage.hpp"
#include "security/SecurityManager.hpp"
#include "security/interfaces/IHashProvider.hpp"
#include "security/interfaces/ISignatureVerifier.hpp"
#include "security/interfaces/ISecureStorage.hpp"
#include "security/interfaces/IIntegrityVerifier.hpp"

// DSP Package Management
#include "dsp/DSPTypes.hpp"
#include "dsp/DSPMetadata.hpp"
#include "dsp/DSPValidator.hpp"
#include "dsp/DSPCache.hpp"
#include "dsp/DSPDependencyGraph.hpp"
#include "dsp/DSPInspector.hpp"
#include "dsp/DSPBuilder.hpp"
#include "dsp/DSPManager.hpp"
#include "dsp/DSPRepository.hpp"
#include "dsp/HardwareProfile.hpp"
#include "dsp/LoaderRepository.hpp"
#include "dsp/VendorQuirk.hpp"

// Performance Profiler
// Performance Profiler
#include "profiler/ProfilerTypes.hpp"
#include "profiler/Profiler.hpp"

// Memory Tracking
// Memory Tracking
#include "memory/MemoryTracker.hpp"

// Stress Testing
// Stress Testing
#include "stress/StressTypes.hpp"
#include "stress/VirtualStressEnvironment.hpp"

// Fault Injection
// Fault Injection
#include "faults/FaultTypes.hpp"
#include "faults/FaultInjector.hpp"

// Telemetry
// Telemetry
#include "telemetry/TelemetryTypes.hpp"
#include "telemetry/TelemetryCollector.hpp"

// API Reference
#include "api/ApiReference.hpp"
