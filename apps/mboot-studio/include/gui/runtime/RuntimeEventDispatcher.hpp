#pragma once

#include "gui/runtime/RuntimeModels.hpp"
#include "gui/runtime/RuntimeTypes.hpp"

#include <QObject>
#include <memory>

namespace mbootcore::runtime {
struct RuntimeEvent;
struct RuntimeStatistics;
struct RuntimeHealth;
} // namespace mbootcore::runtime

namespace gui {
namespace runtime {

struct RuntimeError;

class RuntimeEventDispatcher : public QObject {
    Q_OBJECT
public:
    explicit RuntimeEventDispatcher(QObject *parent = nullptr);
    ~RuntimeEventDispatcher() override;

    void dispatchRuntimeEvent(const mbootcore::runtime::RuntimeEvent& event);
    void dispatchStatisticsUpdate(const mbootcore::runtime::RuntimeStatistics& stats,
                                  const mbootcore::runtime::RuntimeHealth& health,
                                  const std::string& version,
                                  RuntimeBridgeState bridgeState);
    void dispatchDeviceConnected(const DeviceInfoView& device);
    void dispatchDeviceDisconnected(const std::string& connectionPath);
    void dispatchSessionStateChanged(const SessionInfoView& session);
    void dispatchProgressUpdate(const ProgressInfoView& progress);
    void dispatchError(const RuntimeError& error);
    void dispatchNotification(const std::string& title, const std::string& message, ErrorSeverity severity);
    void dispatchBridgeStateChanged(RuntimeBridgeState newState);
    void dispatchDeviceListChanged(const std::vector<DeviceInfoView>& devices);
    void dispatchSessionStatusChanged(const SessionInfoView& session);

    void dispatchFlashOperationChanged(const FlashOperationView& operation);
    void dispatchFlashProgressChanged(const FlashProgressView& progress);
    void dispatchFlashResult(const FlashResultView& result);

    void dispatchDiagnosticsUpdated(const RuntimeDiagnosticsView& diagnostics);
    void dispatchHealthChanged(const RuntimeHealthView& health);
    void dispatchPluginListChanged();
    void dispatchCapabilitiesChanged();

signals:
    void runtimeEventOccurred(uint32_t eventType, const QString& source, const QString& message, bool success);
    void statisticsUpdated(const RuntimeStatusView& status);
    void deviceConnected(const DeviceInfoView& device);
    void deviceDisconnected(const QString& connectionPath);
    void sessionStateChanged(const SessionInfoView& session);
    void progressUpdated(const ProgressInfoView& progress);
    void errorOccurred(const RuntimeError& error);
    void notificationReceived(const QString& title, const QString& message, uint32_t severity);
    void bridgeStateChanged(uint32_t newState);
    void deviceListChanged(const std::vector<DeviceInfoView>& devices);
    void sessionStatusChanged(const SessionInfoView& session);

    void flashOperationChanged(const FlashOperationView& operation);
    void flashProgressChanged(const FlashProgressView& progress);
    void flashResult(const FlashResultView& result);

    void diagnosticsUpdated(const RuntimeDiagnosticsView& diagnostics);
    void healthChanged(const RuntimeHealthView& health);
    void pluginListChanged();
    void capabilitiesChanged();
};

} // namespace runtime
} // namespace gui
