#pragma once

#include <cstdint>
#include <string>

namespace gui {
namespace runtime {

enum class RuntimeBridgeState : uint32_t {
    Uninitialized = 0,
    Initializing,
    Ready,
    Shutdown,
    Error
};

enum class ErrorSeverity : uint32_t {
    Info = 0,
    Warning,
    Error,
    Critical
};

enum class DeviceConnectionStatus : uint32_t {
    Disconnected = 0,
    Connecting,
    Connected,
    Error
};

enum class SessionStatus : uint32_t {
    Idle = 0,
    Busy,
    Reading,
    Writing,
    Erasing,
    Error
};

enum class TransportStatus : uint32_t {
    Closed = 0,
    Opening,
    Open,
    Closing,
    Error
};

enum class FlashStatus : uint32_t {
    Idle = 0,
    Preparing,
    Validating,
    Flashing,
    Verifying,
    Completed,
    Cancelled,
    Failed
};

enum class FlashStage : uint32_t {
    None = 0,
    LoadingPackage,
    ValidatingPackage,
    GeneratingPlan,
    Programming,
    WritingPartitions,
    Verifying,
    Finalizing
};

enum class DiagnosticSeverity : uint32_t {
    Info = 0,
    Warning,
    Error,
    Critical
};

enum class DiagnosticCategory : uint32_t {
    Runtime = 0,
    Memory,
    Transport,
    Pipeline,
    Workflow,
    JobEngine,
    Plugin,
    DSP,
    Vendor,
    Cache,
    Performance,
    Configuration,
    System,
    Security
};

enum class PluginStatus : uint32_t {
    Unloaded = 0,
    Loaded,
    Initialized,
    Enabled,
    Disabled,
    Error
};

enum class CapabilityType : uint32_t {
    USB = 0,
    Serial,
    TCP,
    UDP,
    Crypto,
    FirmwareValidation,
    VendorSupport,
    Diagnostics,
    PluginSystem,
    SecurityFeatures,
    Unknown
};

} // namespace runtime
} // namespace gui
