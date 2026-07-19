#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include "gui/runtime/RuntimeConverters.hpp"
#include "gui/runtime/RuntimeErrorMapper.hpp"

#include <mbootcore/runtime/RuntimeEvents.hpp>
#include <mbootcore/runtime/RuntimeStatistics.hpp>

#include <QMetaType>

Q_DECLARE_METATYPE(gui::runtime::RuntimeError)

namespace gui {
namespace runtime {

namespace {

RuntimeBridgeState s_previousBridgeState = RuntimeBridgeState::Uninitialized;

} // anonymous namespace

RuntimeEventDispatcher::RuntimeEventDispatcher(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<DeviceInfoView>("DeviceInfoView");
    qRegisterMetaType<SessionInfoView>("SessionInfoView");
    qRegisterMetaType<RuntimeStatusView>("RuntimeStatusView");
    qRegisterMetaType<TransportStatusView>("TransportStatusView");
    qRegisterMetaType<ProgressInfoView>("ProgressInfoView");
    qRegisterMetaType<RuntimeError>("RuntimeError");
    qRegisterMetaType<FlashOperationView>("FlashOperationView");
    qRegisterMetaType<FlashProgressView>("FlashProgressView");
    qRegisterMetaType<FlashResultView>("FlashResultView");
    qRegisterMetaType<RuntimeDiagnosticsView>("RuntimeDiagnosticsView");
    qRegisterMetaType<RuntimeHealthView>("RuntimeHealthView");
    qRegisterMetaType<PluginInfoView>("PluginInfoView");
    qRegisterMetaType<CapabilityView>("CapabilityView");
}

RuntimeEventDispatcher::~RuntimeEventDispatcher() = default;

void RuntimeEventDispatcher::dispatchRuntimeEvent(
    const mbootcore::runtime::RuntimeEvent& event)
{
    const auto eventType = static_cast<uint32_t>(event.type);
    emit runtimeEventOccurred(
        eventType,
        QString::fromStdString(event.source),
        QString::fromStdString(event.message),
        event.success);

    switch (event.type) {
        case mbootcore::runtime::RuntimeEventType::DeviceConnected: {
            DeviceInfoView device;
            device.friendlyName = event.source;
            device.connectionPath = event.message;
            device.isValid = true;
            emit deviceConnected(device);
            break;
        }
        case mbootcore::runtime::RuntimeEventType::DeviceDisconnected:
            emit deviceDisconnected(QString::fromStdString(event.message));
            break;
        case mbootcore::runtime::RuntimeEventType::RuntimeStarted:
            dispatchBridgeStateChanged(RuntimeBridgeState::Ready);
            break;
        case mbootcore::runtime::RuntimeEventType::RuntimeStopped:
            dispatchBridgeStateChanged(RuntimeBridgeState::Shutdown);
            break;
        default:
            break;
    }

    if (!event.success && event.error != mbootcore::ErrorCode::Success) {
        RuntimeError error = RuntimeErrorMapper::map(event.error);
        error.source = event.source;
        emit errorOccurred(error);
    }
}

void RuntimeEventDispatcher::dispatchStatisticsUpdate(
    const mbootcore::runtime::RuntimeStatistics& stats,
    const mbootcore::runtime::RuntimeHealth& health,
    const std::string& version,
    RuntimeBridgeState bridgeState)
{
    RuntimeStatusView status = RuntimeConverters::fromCoreStatistics(
        stats, health, version, bridgeState);
    emit statisticsUpdated(status);
}

void RuntimeEventDispatcher::dispatchDeviceConnected(const DeviceInfoView& device)
{
    emit deviceConnected(device);
}

void RuntimeEventDispatcher::dispatchDeviceDisconnected(const std::string& connectionPath)
{
    emit deviceDisconnected(QString::fromStdString(connectionPath));
}

void RuntimeEventDispatcher::dispatchSessionStateChanged(const SessionInfoView& session)
{
    emit sessionStateChanged(session);
}

void RuntimeEventDispatcher::dispatchProgressUpdate(const ProgressInfoView& progress)
{
    emit progressUpdated(progress);
}

void RuntimeEventDispatcher::dispatchError(const RuntimeError& error)
{
    emit errorOccurred(error);
}

void RuntimeEventDispatcher::dispatchNotification(
    const std::string& title, const std::string& message, ErrorSeverity severity)
{
    emit notificationReceived(
        QString::fromStdString(title),
        QString::fromStdString(message),
        static_cast<uint32_t>(severity));
}

void RuntimeEventDispatcher::dispatchBridgeStateChanged(RuntimeBridgeState newState)
{
    if (newState != s_previousBridgeState) {
        s_previousBridgeState = newState;
        emit bridgeStateChanged(static_cast<uint32_t>(newState));
    }
}

void RuntimeEventDispatcher::dispatchDeviceListChanged(
    const std::vector<DeviceInfoView>& devices)
{
    emit deviceListChanged(devices);
}

void RuntimeEventDispatcher::dispatchSessionStatusChanged(const SessionInfoView& session)
{
    emit sessionStatusChanged(session);
}

void RuntimeEventDispatcher::dispatchFlashOperationChanged(const FlashOperationView& operation)
{
    emit flashOperationChanged(operation);
}

void RuntimeEventDispatcher::dispatchFlashProgressChanged(const FlashProgressView& progress)
{
    emit flashProgressChanged(progress);
}

void RuntimeEventDispatcher::dispatchFlashResult(const FlashResultView& result)
{
    emit flashResult(result);
}

void RuntimeEventDispatcher::dispatchDiagnosticsUpdated(const RuntimeDiagnosticsView& diagnostics)
{
    emit diagnosticsUpdated(diagnostics);
}

void RuntimeEventDispatcher::dispatchHealthChanged(const RuntimeHealthView& health)
{
    emit healthChanged(health);
}

void RuntimeEventDispatcher::dispatchPluginListChanged()
{
    emit pluginListChanged();
}

void RuntimeEventDispatcher::dispatchCapabilitiesChanged()
{
    emit capabilitiesChanged();
}

} // namespace runtime
} // namespace gui
