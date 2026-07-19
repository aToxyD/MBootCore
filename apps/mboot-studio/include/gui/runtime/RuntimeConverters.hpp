#pragma once

#include "gui/runtime/RuntimeModels.hpp"
#include "gui/runtime/RuntimeTypes.hpp"

#include <mbootcore/domain/DeviceTypes.hpp>
#include <mbootcore/domain/Types.hpp>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/runtime/RuntimeStatistics.hpp>
#include <mbootcore/runtime/RuntimeHardware.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>

#include <string>

namespace gui {
namespace runtime {

class RuntimeConverters {
public:
    static DeviceInfoView fromCoreDescriptor(
        const mbootcore::discovery::DeviceDescriptor& descriptor);

    static SessionInfoView fromCoreSessionState(
        mbootcore::session::SessionState state,
        const mbootcore::session::SessionStatistics& stats,
        const std::string& sessionId,
        const std::string& deviceName,
        bool connected);

    static RuntimeStatusView fromCoreStatistics(
        const mbootcore::runtime::RuntimeStatistics& stats,
        const mbootcore::runtime::RuntimeHealth& health,
        const std::string& version,
        RuntimeBridgeState bridgeState);

    static TransportStatusView fromCoreTransportStats(
        mbootcore::transport::TransportType type,
        const mbootcore::transport::TransportStatistics& stats,
        mbootcore::transport::TransportState state);

    static ProgressInfoView fromCoreProgress(
        const mbootcore::transport::TransportProgress& progress,
        const std::string& operationName);

    static ProgressInfoView fromGenericProgress(
        const mbootcore::ProgressInfo& progress,
        const std::string& operationName);

    static std::string vendorToString(mbootcore::discovery::Vendor vendor);
    static std::string bootModeToString(mbootcore::discovery::BootMode mode);
    static std::string transportTypeToString(mbootcore::transport::TransportType type);
    static std::string protocolTypeToString(mbootcore::discovery::ProtocolType type);
    static std::string sessionStateToString(mbootcore::session::SessionState state);
    static std::string transportStateToString(mbootcore::transport::TransportState state);

    static FlashOperationView fromFlashStatus(
        FlashStatus status, const std::string& packagePath,
        const std::string& packageName, const std::string& vendor);

    static FlashProgressView fromGenericProgressForFlash(
        const mbootcore::ProgressInfo& progress,
        FlashStage stage);

    static std::string flashStatusToString(FlashStatus status);
    static std::string flashStageToString(FlashStage stage);

    static RuntimeDiagnosticsView fromCoreDiagnostics(
        const mbootcore::runtime::RuntimeStatistics& stats,
        const mbootcore::runtime::RuntimeHealth& health,
        const mbootcore::runtime::HardwareDiagnosticReport& hwReport,
        const std::string& version);

    static RuntimeHealthView fromCoreHealth(
        const mbootcore::runtime::RuntimeHealth& health);

    static DeviceHealthView fromCoreDeviceEntry(
        const mbootcore::runtime::HardwareDeviceEntry& entry);

    static PluginInfoView fromPluginName(const std::string& name);

    static CapabilityView fromCapabilityString(const std::string& capability);

    static std::string diagnosticSeverityToString(DiagnosticSeverity severity);
    static std::string diagnosticCategoryToString(DiagnosticCategory category);
    static std::string pluginStatusToString(PluginStatus status);
    static std::string capabilityTypeToString(CapabilityType type);
    static CapabilityType capabilityTypeFromString(const std::string& name);
};

} // namespace runtime
} // namespace gui
