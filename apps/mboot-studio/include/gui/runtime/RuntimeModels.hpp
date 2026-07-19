#pragma once

#include "gui/runtime/RuntimeTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include <QMetaType>

namespace gui {
namespace runtime {

struct DeviceInfoView {
    std::string friendlyName;
    std::string connectionPath;
    std::string vendorName;
    std::string bootModeName;
    std::string transportName;
    std::string protocolName;
    uint16_t usbVid{0};
    uint16_t usbPid{0};
    std::string serialPort;
    std::string tcpHost;
    uint16_t tcpPort{0};
    DeviceConnectionStatus connectionStatus{DeviceConnectionStatus::Disconnected};
    bool isValid{false};
};

struct DiscoveryResultView {
    std::vector<DeviceInfoView> devices;
    bool success{false};
    std::string errorMessage;
};

struct SessionInfoView {
    std::string sessionId;
    std::string deviceName;
    SessionStatus status{SessionStatus::Idle};
    uint64_t bytesRead{0};
    uint64_t bytesWritten{0};
    uint64_t bytesErased{0};
    double averageReadBps{0.0};
    double averageWriteBps{0.0};
    uint32_t retryCount{0};
    uint32_t failureCount{0};
    bool isConnected{false};
};

struct RuntimeStatusView {
    RuntimeBridgeState bridgeState{RuntimeBridgeState::Uninitialized};
    bool isInitialized{false};
    std::string version;
    uint32_t activeSessions{0};
    uint32_t activeWorkflows{0};
    uint32_t queuedJobs{0};
    uint32_t loadedPlugins{0};
    uint32_t loadedVendors{0};
    uint32_t connectedDevices{0};
    double uptimeSeconds{0.0};
    uint32_t totalErrors{0};
    uint32_t totalRecoveries{0};
};

struct TransportStatusView {
    TransportStatus status{TransportStatus::Closed};
    std::string transportType;
    uint64_t bytesRead{0};
    uint64_t bytesWritten{0};
    uint64_t readOps{0};
    uint64_t writeOps{0};
    uint32_t reconnectCount{0};
    uint32_t timeoutCount{0};
    uint32_t errorCount{0};
    double averageLatency{0.0};
    double peakThroughput{0.0};
};

struct ProgressInfoView {
    uint64_t transferredBytes{0};
    uint64_t totalBytes{0};
    double percentage{0.0};
    int64_t elapsedMs{0};
    int64_t etaMs{0};
    std::string operationName;
};

struct FlashOperationView {
    std::string packagePath;
    std::string packageName;
    std::string vendor;
    std::string version;
    uint64_t packageSize{0};
    uint32_t imageCount{0};
    uint32_t stepCount{0};
    uint64_t totalBytes{0};
    FlashStatus status{FlashStatus::Idle};
    bool canStart{false};
    bool canCancel{false};
};

struct FlashProgressView {
    FlashStatus status{FlashStatus::Idle};
    FlashStage stage{FlashStage::None};
    std::string stageName;
    std::string currentOperation;
    uint64_t transferredBytes{0};
    uint64_t totalBytes{0};
    double percentage{0.0};
    int64_t elapsedMs{0};
    int64_t etaMs{0};
    double speedBps{0.0};
    uint32_t currentStep{0};
    uint32_t totalSteps{0};
};

struct FlashResultView {
    bool success{false};
    bool verified{false};
    FlashStatus finalStatus{FlashStatus::Idle};
    std::string packageName;
    std::string deviceName;
    int64_t elapsedMs{0};
    uint64_t bytesWritten{0};
    std::string errorMessage;
    std::string recoveryRecommendation;
};

struct DiagnosticEntryView {
    std::string id;
    std::string title;
    std::string description;
    DiagnosticSeverity severity{DiagnosticSeverity::Info};
    DiagnosticCategory category{DiagnosticCategory::Runtime};
    std::string status;
    std::vector<std::string> details;
};

struct DiagnosticReportView {
    std::string timestamp;
    std::string sessionId;
    std::string overallHealth;
    std::vector<DiagnosticEntryView> issues;
    std::vector<std::string> recommendations;
    std::string summary;
};

struct RuntimeHealthView {
    std::string overallHealth;
    uint32_t activeSessions{0};
    uint32_t activeWorkflows{0};
    uint32_t queuedJobs{0};
    uint32_t loadedPlugins{0};
    uint32_t loadedVendors{0};
    uint32_t connectedDevices{0};
    uint64_t memoryUsageBytes{0};
    uint32_t threadCount{0};
    std::string transportState;
    double uptimeSeconds{0.0};
};

struct DeviceHealthView {
    std::string name;
    std::string vendor;
    std::string transportType;
    std::string bootMode;
    std::string protocol;
    bool connected{false};
    uint16_t vendorId{0};
    uint16_t productId{0};
};

struct RuntimeDiagnosticsView {
    RuntimeHealthView health;
    std::vector<DeviceHealthView> devices;
    uint32_t totalErrors{0};
    uint32_t totalRecoveries{0};
    uint32_t totalTimeouts{0};
    uint32_t totalDisconnects{0};
    double averageFlashSpeedBps{0.0};
    double averageReadSpeedBps{0.0};
    double averageWriteSpeedBps{0.0};
    uint32_t jobsExecuted{0};
    uint32_t workflowsExecuted{0};
    std::string version;
    std::string osInfo;
    std::vector<std::string> failures;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
};

struct PluginInfoView {
    std::string name;
    std::string version;
    std::string apiVersion;
    std::string vendor;
    std::string author;
    std::string description;
    std::string license;
    PluginStatus status{PluginStatus::Unloaded};
    bool enabled{false};
    std::vector<std::string> capabilities;
    std::vector<std::string> supportedProtocols;
    std::vector<std::string> dependencies;
    uint32_t priority{0};
};

struct PluginCapabilityView {
    CapabilityType type{CapabilityType::Unknown};
    std::string name;
    std::string description;
    bool available{false};
};

struct CapabilityView {
    std::string name;
    std::string description;
    bool available{false};
    std::vector<std::string> details;
};

} // namespace runtime
} // namespace gui

Q_DECLARE_METATYPE(gui::runtime::DeviceInfoView)
Q_DECLARE_METATYPE(gui::runtime::SessionInfoView)
Q_DECLARE_METATYPE(gui::runtime::RuntimeStatusView)
Q_DECLARE_METATYPE(gui::runtime::TransportStatusView)
Q_DECLARE_METATYPE(gui::runtime::ProgressInfoView)
Q_DECLARE_METATYPE(gui::runtime::FlashOperationView)
Q_DECLARE_METATYPE(gui::runtime::FlashProgressView)
Q_DECLARE_METATYPE(gui::runtime::FlashResultView)
Q_DECLARE_METATYPE(gui::runtime::DiagnosticEntryView)
Q_DECLARE_METATYPE(gui::runtime::DiagnosticReportView)
Q_DECLARE_METATYPE(gui::runtime::RuntimeHealthView)
Q_DECLARE_METATYPE(gui::runtime::DeviceHealthView)
Q_DECLARE_METATYPE(gui::runtime::RuntimeDiagnosticsView)
Q_DECLARE_METATYPE(gui::runtime::PluginInfoView)
Q_DECLARE_METATYPE(gui::runtime::PluginCapabilityView)
Q_DECLARE_METATYPE(gui::runtime::CapabilityView)
