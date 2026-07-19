#pragma once

#include "gui/runtime/RuntimeTypes.hpp"
#include "gui/runtime/RuntimeModels.hpp"

#include <mbootcore/domain/Error.hpp>

#include <QObject>
#include <QFuture>
#include <memory>

namespace mbootcore::runtime {
class Runtime;
struct RuntimeEvent;
struct RuntimeStatistics;
struct RuntimeHealth;
} // namespace mbootcore::runtime

namespace gui {
namespace runtime {

class RuntimeEventDispatcher;

class RuntimeBridge : public QObject {
    Q_OBJECT
public:
    explicit RuntimeBridge(QObject *parent = nullptr);
    ~RuntimeBridge() override;

    RuntimeBridge(const RuntimeBridge&) = delete;
    RuntimeBridge& operator=(const RuntimeBridge&) = delete;
    RuntimeBridge(RuntimeBridge&&) noexcept;
    RuntimeBridge& operator=(RuntimeBridge&&) noexcept;

    mbootcore::runtime::Runtime* runtime() const noexcept;
    bool isInitialized() const noexcept;
    RuntimeBridgeState state() const noexcept;

    mbootcore::Result<void> initialize();
    void shutdown() noexcept;

    RuntimeEventDispatcher* eventDispatcher() const noexcept;

    RuntimeStatusView currentStatus() const;
    SessionInfoView currentSessionStatus() const;
    FlashOperationView currentFlashStatus() const;

    void discoverDevices();
    void connectDevice(const std::string& connectionPath);
    void disconnectDevice();

    void loadFirmwarePackage(const std::string& path);
    void validateFirmware();
    void startFlash();
    void cancelFlash();

    RuntimeDiagnosticsView runtimeDiagnostics() const;
    RuntimeHealthView deviceDiagnostics() const;
    void refreshDiagnostics();
    std::vector<PluginInfoView> enumeratePlugins() const;
    void unloadPlugin(const std::string& name);
    PluginInfoView pluginInformation(const std::string& name) const;
    std::vector<CapabilityView> pluginCapabilities() const;
    void refreshPluginList();
    std::vector<CapabilityView> runtimeCapabilities() const;

public slots:
    void refreshDeviceList();
    void refreshSessionInfo();
    void refreshFlashStatus();

signals:
    void bridgeInitialized();
    void bridgeShutdown();
    void deviceListChanged(const std::vector<DeviceInfoView>& devices);
    void discoveryFailed(const QString& error);
    void connectionFailed(const QString& error);
    void connectionSucceeded(const QString& connectionPath);
    void disconnected();

    void packageLoaded(const QString& path, bool success);
    void firmwareValidated(bool valid, const QString& message);
    void flashStarted();
    void flashProgressChanged(const FlashProgressView& progress);
    void flashCompleted(const FlashResultView& result);
    void flashCancelled();
    void flashFailed(const QString& error);

    void diagnosticsUpdated(const RuntimeDiagnosticsView& diagnostics);
    void healthChanged(const RuntimeHealthView& health);
    void pluginListChanged();
    void pluginLoaded(const QString& name);
    void pluginUnloaded(const QString& name);
    void capabilitiesChanged();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace runtime
} // namespace gui
