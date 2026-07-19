#include "gui/runtime/RuntimeConverters.hpp"

namespace gui {
namespace runtime {

DeviceInfoView RuntimeConverters::fromCoreDescriptor(
    const mbootcore::discovery::DeviceDescriptor& descriptor)
{
    DeviceInfoView view;
    view.friendlyName = descriptor.friendlyName;
    view.connectionPath = descriptor.connectionPath;
    view.vendorName = vendorToString(descriptor.vendor);
    view.bootModeName = bootModeToString(descriptor.bootMode);
    view.transportName = transportTypeToString(descriptor.transport);
    view.protocolName = protocolTypeToString(descriptor.protocolHint);
    view.usbVid = descriptor.usbVid;
    view.usbPid = descriptor.usbPid;
    view.serialPort = descriptor.serialPort;
    view.tcpHost = descriptor.tcpHost;
    view.tcpPort = descriptor.tcpPort;
    view.isValid = descriptor.isValid();
    return view;
}

SessionInfoView RuntimeConverters::fromCoreSessionState(
    mbootcore::session::SessionState state,
    const mbootcore::session::SessionStatistics& stats,
    const std::string& sessionId,
    const std::string& deviceName,
    bool connected)
{
    SessionInfoView view;
    view.sessionId = sessionId;
    view.deviceName = deviceName;
    view.isConnected = connected;
    view.bytesRead = stats.bytesRead;
    view.bytesWritten = stats.bytesWritten;
    view.bytesErased = stats.bytesErased;
    view.averageReadBps = stats.averageReadBps;
    view.averageWriteBps = stats.averageWriteBps;
    view.retryCount = stats.retries;
    view.failureCount = stats.failures;

    switch (state) {
        case mbootcore::session::SessionState::Disconnected:
        case mbootcore::session::SessionState::Discovered:
        case mbootcore::session::SessionState::Negotiated:
            view.status = SessionStatus::Idle;
            break;
        case mbootcore::session::SessionState::Connected:
        case mbootcore::session::SessionState::Initializing:
        case mbootcore::session::SessionState::Ready:
            view.status = SessionStatus::Idle;
            break;
        case mbootcore::session::SessionState::Busy:
        case mbootcore::session::SessionState::Connecting:
            view.status = SessionStatus::Busy;
            break;
        case mbootcore::session::SessionState::Reading:
            view.status = SessionStatus::Reading;
            break;
        case mbootcore::session::SessionState::Writing:
            view.status = SessionStatus::Writing;
            break;
        case mbootcore::session::SessionState::Erasing:
            view.status = SessionStatus::Erasing;
            break;
        case mbootcore::session::SessionState::Resetting:
        case mbootcore::session::SessionState::Finished:
            view.status = SessionStatus::Idle;
            break;
        case mbootcore::session::SessionState::Error:
        case mbootcore::session::SessionState::Cancelled:
            view.status = SessionStatus::Error;
            break;
    }

    return view;
}

RuntimeStatusView RuntimeConverters::fromCoreStatistics(
    const mbootcore::runtime::RuntimeStatistics& stats,
    const mbootcore::runtime::RuntimeHealth& health,
    const std::string& version,
    RuntimeBridgeState bridgeState)
{
    RuntimeStatusView view;
    view.bridgeState = bridgeState;
    view.isInitialized = (bridgeState == RuntimeBridgeState::Ready);
    view.version = version;
    view.activeSessions = health.activeSessions;
    view.activeWorkflows = health.activeWorkflows;
    view.queuedJobs = health.queuedJobs;
    view.loadedPlugins = health.loadedPlugins;
    view.loadedVendors = health.loadedVendors;
    view.connectedDevices = health.connectedDevices;
    view.uptimeSeconds = stats.uptimeSeconds;
    view.totalErrors = stats.totalErrors;
    view.totalRecoveries = stats.totalRecoveries;
    return view;
}

TransportStatusView RuntimeConverters::fromCoreTransportStats(
    mbootcore::transport::TransportType type,
    const mbootcore::transport::TransportStatistics& stats,
    mbootcore::transport::TransportState state)
{
    TransportStatusView view;
    view.transportType = transportTypeToString(type);
    view.bytesRead = stats.bytesRead;
    view.bytesWritten = stats.bytesWritten;
    view.readOps = stats.readOperations;
    view.writeOps = stats.writeOperations;
    view.reconnectCount = stats.reconnectCount;
    view.timeoutCount = stats.timeoutCount;
    view.errorCount = stats.errorCount;
    view.averageLatency = stats.averageLatency;
    view.peakThroughput = stats.peakThroughput;

    switch (state) {
        case mbootcore::transport::TransportState::Closed:
            view.status = TransportStatus::Closed;
            break;
        case mbootcore::transport::TransportState::Opening:
            view.status = TransportStatus::Opening;
            break;
        case mbootcore::transport::TransportState::Open:
            view.status = TransportStatus::Open;
            break;
        case mbootcore::transport::TransportState::Closing:
            view.status = TransportStatus::Closing;
            break;
        case mbootcore::transport::TransportState::Error:
            view.status = TransportStatus::Error;
            break;
    }

    return view;
}

ProgressInfoView RuntimeConverters::fromCoreProgress(
    const mbootcore::transport::TransportProgress& progress,
    const std::string& operationName)
{
    ProgressInfoView view;
    view.transferredBytes = progress.transferredBytes;
    view.totalBytes = progress.totalBytes;
    view.percentage = progress.percentage;
    view.elapsedMs = progress.elapsed.count();
    view.etaMs = progress.eta.count();
    view.operationName = operationName;
    return view;
}

ProgressInfoView RuntimeConverters::fromGenericProgress(
    const mbootcore::ProgressInfo& progress,
    const std::string& operationName)
{
    ProgressInfoView view;
    view.transferredBytes = progress.transferredBytes;
    view.totalBytes = progress.totalBytes;
    view.percentage = progress.percentage;
    view.elapsedMs = 0;
    view.etaMs = static_cast<int64_t>(progress.estimatedSeconds * 1000.0);
    view.operationName = operationName.empty() ? progress.currentOperation : operationName;
    return view;
}

std::string RuntimeConverters::vendorToString(mbootcore::discovery::Vendor vendor)
{
    switch (vendor) {
        case mbootcore::discovery::Vendor::Unknown:    return "Unknown";
        case mbootcore::discovery::Vendor::Qualcomm:   return "Qualcomm";
        case mbootcore::discovery::Vendor::MediaTek:   return "MediaTek";
        case mbootcore::discovery::Vendor::UNISOC:     return "UNISOC";
        case mbootcore::discovery::Vendor::Samsung:    return "Samsung";
        case mbootcore::discovery::Vendor::Rockchip:   return "Rockchip";
        case mbootcore::discovery::Vendor::Spreadtrum: return "Spreadtrum";
        case mbootcore::discovery::Vendor::Apple:      return "Apple";
        case mbootcore::discovery::Vendor::Google:     return "Google";
        case mbootcore::discovery::Vendor::Huawei:     return "Huawei";
        case mbootcore::discovery::Vendor::Custom:     return "Custom";
    }
    return "Unknown";
}

std::string RuntimeConverters::bootModeToString(mbootcore::discovery::BootMode mode)
{
    switch (mode) {
        case mbootcore::discovery::BootMode::Unknown:      return "Unknown";
        case mbootcore::discovery::BootMode::BootROM:      return "BootROM";
        case mbootcore::discovery::BootMode::EDL:          return "EDL";
        case mbootcore::discovery::BootMode::Firehose:     return "Firehose";
        case mbootcore::discovery::BootMode::Fastboot:     return "Fastboot";
        case mbootcore::discovery::BootMode::ADB:          return "ADB";
        case mbootcore::discovery::BootMode::Recovery:     return "Recovery";
        case mbootcore::discovery::BootMode::DownloadMode: return "Download Mode";
        case mbootcore::discovery::BootMode::Preloader:    return "Preloader";
        case mbootcore::discovery::BootMode::BROM:         return "BROM";
        case mbootcore::discovery::BootMode::Normal:       return "Normal";
        case mbootcore::discovery::BootMode::Download:     return "Download";
        case mbootcore::discovery::BootMode::Custom:       return "Custom";
    }
    return "Unknown";
}

std::string RuntimeConverters::transportTypeToString(mbootcore::transport::TransportType type)
{
    switch (type) {
        case mbootcore::transport::TransportType::Unknown:  return "Unknown";
        case mbootcore::transport::TransportType::Mock:     return "Mock";
        case mbootcore::transport::TransportType::Virtual:  return "Virtual";
        case mbootcore::transport::TransportType::USB:      return "USB";
        case mbootcore::transport::TransportType::Serial:   return "Serial";
        case mbootcore::transport::TransportType::TCP:      return "TCP";
        case mbootcore::transport::TransportType::UDP:      return "UDP";
        case mbootcore::transport::TransportType::Bluetooth: return "Bluetooth";
        case mbootcore::transport::TransportType::HID:      return "HID";
    }
    return "Unknown";
}

std::string RuntimeConverters::protocolTypeToString(mbootcore::discovery::ProtocolType type)
{
    switch (type) {
        case mbootcore::discovery::ProtocolType::Unknown:       return "Unknown";
        case mbootcore::discovery::ProtocolType::Sahara:        return "Sahara";
        case mbootcore::discovery::ProtocolType::Firehose:      return "Firehose";
        case mbootcore::discovery::ProtocolType::Fastboot:      return "Fastboot";
        case mbootcore::discovery::ProtocolType::MediaTekBROM:  return "MediaTek BROM";
        case mbootcore::discovery::ProtocolType::MediaTekDA:    return "MediaTek DA";
        case mbootcore::discovery::ProtocolType::UNISOCBootROM: return "UNISOC BootROM";
        case mbootcore::discovery::ProtocolType::UNISOCFDL:     return "UNISOC FDL";
        case mbootcore::discovery::ProtocolType::USBStream:     return "USB Stream";
        case mbootcore::discovery::ProtocolType::Custom:        return "Custom";
    }
    return "Unknown";
}

std::string RuntimeConverters::sessionStateToString(mbootcore::session::SessionState state)
{
    switch (state) {
        case mbootcore::session::SessionState::Disconnected: return "Disconnected";
        case mbootcore::session::SessionState::Discovered:   return "Discovered";
        case mbootcore::session::SessionState::Negotiated:   return "Negotiated";
        case mbootcore::session::SessionState::Connected:    return "Connected";
        case mbootcore::session::SessionState::Initializing: return "Initializing";
        case mbootcore::session::SessionState::Ready:        return "Ready";
        case mbootcore::session::SessionState::Busy:         return "Busy";
        case mbootcore::session::SessionState::Reading:      return "Reading";
        case mbootcore::session::SessionState::Writing:      return "Writing";
        case mbootcore::session::SessionState::Erasing:      return "Erasing";
        case mbootcore::session::SessionState::Resetting:    return "Resetting";
        case mbootcore::session::SessionState::Finished:     return "Finished";
        case mbootcore::session::SessionState::Error:        return "Error";
        case mbootcore::session::SessionState::Cancelled:    return "Cancelled";
        case mbootcore::session::SessionState::Connecting:   return "Connecting";
    }
    return "Unknown";
}

std::string RuntimeConverters::transportStateToString(mbootcore::transport::TransportState state)
{
    switch (state) {
        case mbootcore::transport::TransportState::Closed:  return "Closed";
        case mbootcore::transport::TransportState::Opening: return "Opening";
        case mbootcore::transport::TransportState::Open:    return "Open";
        case mbootcore::transport::TransportState::Closing: return "Closing";
        case mbootcore::transport::TransportState::Error:   return "Error";
    }
    return "Unknown";
}

FlashOperationView RuntimeConverters::fromFlashStatus(
    FlashStatus status, const std::string& packagePath,
    const std::string& packageName, const std::string& vendor)
{
    FlashOperationView view;
    view.packagePath = packagePath;
    view.packageName = packageName;
    view.vendor = vendor;
    view.status = status;
    view.canStart = (status == FlashStatus::Idle || status == FlashStatus::Completed
                     || status == FlashStatus::Failed || status == FlashStatus::Cancelled);
    view.canCancel = (status == FlashStatus::Flashing || status == FlashStatus::Preparing
                      || status == FlashStatus::Validating);
    return view;
}

FlashProgressView RuntimeConverters::fromGenericProgressForFlash(
    const mbootcore::ProgressInfo& progress,
    FlashStage stage)
{
    FlashProgressView view;
    view.stage = stage;
    view.stageName = flashStageToString(stage);
    view.currentOperation = progress.currentOperation;
    view.transferredBytes = progress.transferredBytes;
    view.totalBytes = progress.totalBytes;
    view.percentage = progress.percentage;
    view.speedBps = progress.speedBps;
    view.elapsedMs = 0;
    view.etaMs = static_cast<int64_t>(progress.estimatedSeconds * 1000.0);
    return view;
}

std::string RuntimeConverters::flashStatusToString(FlashStatus status)
{
    switch (status) {
        case FlashStatus::Idle:       return "Idle";
        case FlashStatus::Preparing:  return "Preparing";
        case FlashStatus::Validating: return "Validating";
        case FlashStatus::Flashing:   return "Flashing";
        case FlashStatus::Verifying:  return "Verifying";
        case FlashStatus::Completed:  return "Completed";
        case FlashStatus::Cancelled:  return "Cancelled";
        case FlashStatus::Failed:     return "Failed";
    }
    return "Unknown";
}

std::string RuntimeConverters::flashStageToString(FlashStage stage)
{
    switch (stage) {
        case FlashStage::None:              return "None";
        case FlashStage::LoadingPackage:    return "Loading Package";
        case FlashStage::ValidatingPackage: return "Validating Package";
        case FlashStage::GeneratingPlan:    return "Generating Flash Plan";
        case FlashStage::Programming:       return "Programming";
        case FlashStage::WritingPartitions: return "Writing Partitions";
        case FlashStage::Verifying:         return "Verifying";
        case FlashStage::Finalizing:        return "Finalizing";
    }
    return "Unknown";
}

RuntimeDiagnosticsView RuntimeConverters::fromCoreDiagnostics(
    const mbootcore::runtime::RuntimeStatistics& stats,
    const mbootcore::runtime::RuntimeHealth& health,
    const mbootcore::runtime::HardwareDiagnosticReport& hwReport,
    const std::string& version)
{
    RuntimeDiagnosticsView view;
    view.health = fromCoreHealth(health);
    view.version = version;
    view.totalErrors = stats.totalErrors;
    view.totalRecoveries = stats.totalRecoveries;
    view.totalTimeouts = stats.totalTimeouts;
    view.totalDisconnects = stats.totalDisconnects;
    view.averageFlashSpeedBps = stats.averageFlashSpeedBps;
    view.averageReadSpeedBps = stats.averageReadSpeedBps;
    view.averageWriteSpeedBps = stats.averageWriteSpeedBps;
    view.jobsExecuted = stats.jobsExecuted;
    view.workflowsExecuted = stats.workflowsExecuted;
    view.osInfo = hwReport.osVersion;
    view.failures = hwReport.failures;
    view.warnings = hwReport.warnings;
    view.recommendations = hwReport.recommendations;
    for (const auto& entry : hwReport.detectedDevices) {
        view.devices.push_back(fromCoreDeviceEntry(entry));
    }
    return view;
}

RuntimeHealthView RuntimeConverters::fromCoreHealth(
    const mbootcore::runtime::RuntimeHealth& health)
{
    RuntimeHealthView view;
    view.activeSessions = health.activeSessions;
    view.activeWorkflows = health.activeWorkflows;
    view.queuedJobs = health.queuedJobs;
    view.loadedPlugins = health.loadedPlugins;
    view.loadedVendors = health.loadedVendors;
    view.connectedDevices = health.connectedDevices;
    view.memoryUsageBytes = health.memoryUsageBytes;
    view.threadCount = health.threadCount;
    view.transportState = health.transportState;
    view.uptimeSeconds = health.uptimeSeconds;
    return view;
}

DeviceHealthView RuntimeConverters::fromCoreDeviceEntry(
    const mbootcore::runtime::HardwareDeviceEntry& entry)
{
    DeviceHealthView view;
    view.name = entry.name;
    view.vendor = entry.vendor;
    view.transportType = entry.transportType;
    view.bootMode = entry.bootMode;
    view.protocol = entry.protocol;
    view.connected = entry.connected;
    view.vendorId = entry.vendorId;
    view.productId = entry.productId;
    return view;
}

PluginInfoView RuntimeConverters::fromPluginName(const std::string& name)
{
    PluginInfoView view;
    view.name = name;
    view.status = PluginStatus::Loaded;
    return view;
}

CapabilityView RuntimeConverters::fromCapabilityString(const std::string& capability)
{
    CapabilityView view;
    view.name = capability;
    view.available = true;
    auto type = capabilityTypeFromString(capability);
    view.description = capabilityTypeToString(type);
    return view;
}

std::string RuntimeConverters::diagnosticSeverityToString(DiagnosticSeverity severity)
{
    switch (severity) {
        case DiagnosticSeverity::Info:     return "Info";
        case DiagnosticSeverity::Warning:  return "Warning";
        case DiagnosticSeverity::Error:    return "Error";
        case DiagnosticSeverity::Critical: return "Critical";
    }
    return "Unknown";
}

std::string RuntimeConverters::diagnosticCategoryToString(DiagnosticCategory category)
{
    switch (category) {
        case DiagnosticCategory::Runtime:       return "Runtime";
        case DiagnosticCategory::Memory:        return "Memory";
        case DiagnosticCategory::Transport:     return "Transport";
        case DiagnosticCategory::Pipeline:      return "Pipeline";
        case DiagnosticCategory::Workflow:      return "Workflow";
        case DiagnosticCategory::JobEngine:     return "Job Engine";
        case DiagnosticCategory::Plugin:        return "Plugin";
        case DiagnosticCategory::DSP:           return "DSP";
        case DiagnosticCategory::Vendor:        return "Vendor";
        case DiagnosticCategory::Cache:         return "Cache";
        case DiagnosticCategory::Performance:   return "Performance";
        case DiagnosticCategory::Configuration: return "Configuration";
        case DiagnosticCategory::System:        return "System";
        case DiagnosticCategory::Security:      return "Security";
    }
    return "Unknown";
}

std::string RuntimeConverters::pluginStatusToString(PluginStatus status)
{
    switch (status) {
        case PluginStatus::Unloaded:    return "Unloaded";
        case PluginStatus::Loaded:      return "Loaded";
        case PluginStatus::Initialized: return "Initialized";
        case PluginStatus::Enabled:     return "Enabled";
        case PluginStatus::Disabled:    return "Disabled";
        case PluginStatus::Error:       return "Error";
    }
    return "Unknown";
}

std::string RuntimeConverters::capabilityTypeToString(CapabilityType type)
{
    switch (type) {
        case CapabilityType::USB:                return "USB";
        case CapabilityType::Serial:             return "Serial";
        case CapabilityType::TCP:                return "TCP";
        case CapabilityType::UDP:                return "UDP";
        case CapabilityType::Crypto:             return "Crypto";
        case CapabilityType::FirmwareValidation: return "Firmware Validation";
        case CapabilityType::VendorSupport:      return "Vendor Support";
        case CapabilityType::Diagnostics:        return "Diagnostics";
        case CapabilityType::PluginSystem:       return "Plugin System";
        case CapabilityType::SecurityFeatures:   return "Security Features";
        case CapabilityType::Unknown:            return "Unknown";
    }
    return "Unknown";
}

CapabilityType RuntimeConverters::capabilityTypeFromString(const std::string& name)
{
    if (name.find("usb") != std::string::npos || name.find("USB") != std::string::npos)
        return CapabilityType::USB;
    if (name.find("serial") != std::string::npos || name.find("Serial") != std::string::npos)
        return CapabilityType::Serial;
    if (name.find("tcp") != std::string::npos || name.find("TCP") != std::string::npos)
        return CapabilityType::TCP;
    if (name.find("udp") != std::string::npos || name.find("UDP") != std::string::npos)
        return CapabilityType::UDP;
    if (name.find("crypto") != std::string::npos || name.find("Crypto") != std::string::npos)
        return CapabilityType::Crypto;
    if (name.find("firmware") != std::string::npos || name.find("Firmware") != std::string::npos)
        return CapabilityType::FirmwareValidation;
    if (name.find("vendor") != std::string::npos || name.find("Vendor") != std::string::npos)
        return CapabilityType::VendorSupport;
    if (name.find("diagnostic") != std::string::npos || name.find("Diagnostic") != std::string::npos)
        return CapabilityType::Diagnostics;
    if (name.find("plugin") != std::string::npos || name.find("Plugin") != std::string::npos)
        return CapabilityType::PluginSystem;
    if (name.find("security") != std::string::npos || name.find("Security") != std::string::npos)
        return CapabilityType::SecurityFeatures;
    return CapabilityType::Unknown;
}

} // namespace runtime
} // namespace gui
