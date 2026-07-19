#include "gui/runtime/RuntimeBridge.hpp"
#include "gui/runtime/RuntimeEventDispatcher.hpp"
#include "gui/runtime/RuntimeConverters.hpp"
#include "gui/runtime/RuntimeErrorMapper.hpp"

#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/RuntimeObserver.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>

#include <QCoreApplication>
#include <QMetaType>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace gui {
namespace runtime {

class RuntimeBridge::Impl : private mbootcore::runtime::RuntimeObserver {
public:
    explicit Impl(RuntimeBridge* owner)
        : m_owner(owner)
        , m_dispatcher(new RuntimeEventDispatcher(owner))
    {
    }

    ~Impl()
    {
        unregisterObserver();
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    mbootcore::Result<void> initialize()
    {
        m_runtime = std::make_unique<mbootcore::runtime::Runtime>();
        auto result = m_runtime->initialize();
        if (result.isError()) {
            m_runtime.reset();
            return result;
        }

        m_runtime->addObserver(this);
        m_state = RuntimeBridgeState::Ready;

        m_dispatcher->dispatchBridgeStateChanged(m_state);
        m_dispatcher->dispatchNotification(
            "Runtime", "Runtime initialized successfully", ErrorSeverity::Info);

        return mbootcore::Result<void>::Ok();
    }

    void shutdown() noexcept
    {
        if (m_state == RuntimeBridgeState::Uninitialized ||
            m_state == RuntimeBridgeState::Shutdown) {
            return;
        }

        waitForPendingOperations();
        unregisterObserver();
        m_state = RuntimeBridgeState::Shutdown;

        if (m_runtime) {
            m_runtime->shutdown();
            m_runtime.reset();
        }

        m_discoveredDescriptors.clear();
        m_dispatcher->dispatchBridgeStateChanged(m_state);
    }

    mbootcore::runtime::Runtime* runtime() const noexcept
    {
        return m_runtime.get();
    }

    RuntimeBridgeState state() const noexcept
    {
        return m_state;
    }

    RuntimeEventDispatcher* dispatcher() const noexcept
    {
        return m_dispatcher;
    }

    RuntimeStatusView currentStatus() const
    {
        if (!m_runtime || m_state == RuntimeBridgeState::Uninitialized) {
            RuntimeStatusView view;
            view.bridgeState = m_state;
            view.isInitialized = false;
            return view;
        }

        auto stats = m_runtime->statistics();
        auto health = m_runtime->health();
        auto version = m_runtime->version();
        return RuntimeConverters::fromCoreStatistics(stats, health, version, m_state);
    }

    SessionInfoView sessionStatus() const
    {
        if (!m_runtime || m_state == RuntimeBridgeState::Uninitialized) {
            return {};
        }

        auto *session = m_runtime->activeSession();
        if (!session) {
            SessionInfoView view;
            view.isConnected = m_runtime->isConnected();
            return view;
        }

        auto stats = session->statistics();
        auto deviceName = session->descriptor().friendlyName.empty()
            ? session->descriptor().connectionPath
            : session->descriptor().friendlyName;
        return RuntimeConverters::fromCoreSessionState(
            session->state(), stats, session->sessionId(),
            deviceName, session->isConnected());
    }

    void discoverDevices()
    {
        if (!m_runtime) return;

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();

        auto future = QtConcurrent::run([rt]() -> std::vector<mbootcore::discovery::DeviceDescriptor> {
            auto result = rt->discover(std::chrono::seconds(3));
            if (result.isError()) {
                return {};
            }
            return result.value();
        });

        auto *watcher = new QFutureWatcher<std::vector<mbootcore::discovery::DeviceDescriptor>>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher]() {
                auto descriptors = watcher->result();
                watcher->deleteLater();
                self->m_impl->m_discoveredDescriptors = descriptors;

                std::vector<DeviceInfoView> views;
                views.reserve(descriptors.size());
                for (const auto& desc : descriptors) {
                    views.push_back(RuntimeConverters::fromCoreDescriptor(desc));
                }

                QMetaObject::invokeMethod(dispatcher,
                    [dispatcher, views]() {
                        dispatcher->dispatchDeviceListChanged(views);
                    }, Qt::QueuedConnection);

                self->m_impl->updateDeviceListFromRuntime();
                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void connectDevice(const std::string& connectionPath)
    {
        if (!m_runtime) return;

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();
        auto path = connectionPath;

        auto future = QtConcurrent::run([rt, path]() -> bool {
            auto descriptors = rt->discover(std::chrono::seconds(2));
            if (descriptors.isError()) return false;

            for (const auto& desc : descriptors.value()) {
                if (desc.connectionPath == path) {
                    auto result = rt->connect(desc);
                    return result.isOk();
                }
            }
            return false;
        });

        auto *watcher = new QFutureWatcher<bool>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher, path]() {
                bool success = watcher->result();
                watcher->deleteLater();

                if (success) {
                    QMetaObject::invokeMethod(self, [self, path]() {
                        self->m_impl->updateDeviceListFromRuntime();
                        self->m_impl->updateSessionFromRuntime();
                        emit self->connectionSucceeded(QString::fromStdString(path));
                    }, Qt::QueuedConnection);
                } else {
                    QMetaObject::invokeMethod(dispatcher,
                        [dispatcher, path]() {
                            RuntimeError error;
                            error.coreCode = mbootcore::ErrorCode::DeviceNotFound;
                            error.severity = ErrorSeverity::Error;
                            error.title = "Connection Failed";
                            error.message = "Failed to connect to device: " + path;
                            error.source = "RuntimeBridge";
                            dispatcher->dispatchError(error);
                        }, Qt::QueuedConnection);

                    QMetaObject::invokeMethod(self, [self, path]() {
                        emit self->connectionFailed(
                            QString::fromStdString("Failed to connect to " + path));
                    }, Qt::QueuedConnection);
                }

                self->m_impl->updateDeviceListFromRuntime();
                self->m_impl->updateSessionFromRuntime();
                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void disconnectDevice()
    {
        if (!m_runtime) return;

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();

        auto future = QtConcurrent::run([rt]() {
            rt->disconnect();
        });

        auto *watcher = new QFutureWatcher<void>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher]() {
                watcher->deleteLater();

                self->m_impl->updateDeviceListFromRuntime();
                self->m_impl->updateSessionFromRuntime();

                QMetaObject::invokeMethod(self, [self]() {
                    emit self->disconnected();
                }, Qt::QueuedConnection);

                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void waitForPendingOperations()
    {
        while (m_pendingAsyncTasks.load() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void updateDeviceListFromRuntime()
    {
        if (!m_runtime || m_state == RuntimeBridgeState::Uninitialized) {
            return;
        }

        std::vector<DeviceInfoView> views;
        views.reserve(m_discoveredDescriptors.size());

        bool connected = m_runtime->isConnected();
        auto *session = m_runtime->activeSession();
        std::string connectedPath;
        if (session && connected) {
            connectedPath = session->descriptor().connectionPath;
        }

        for (const auto& desc : m_discoveredDescriptors) {
            auto view = RuntimeConverters::fromCoreDescriptor(desc);
            if (!connectedPath.empty() && desc.connectionPath == connectedPath) {
                view.connectionStatus = DeviceConnectionStatus::Connected;
            }
            views.push_back(std::move(view));
        }

        auto *dispatcher = m_dispatcher;
        QMetaObject::invokeMethod(dispatcher,
            [dispatcher, views]() {
                dispatcher->dispatchDeviceListChanged(views);
            }, Qt::QueuedConnection);
    }

    void updateSessionFromRuntime()
    {
        if (!m_runtime || m_state == RuntimeBridgeState::Uninitialized) {
            return;
        }

        auto session = sessionStatus();
        auto *dispatcher = m_dispatcher;
        QMetaObject::invokeMethod(dispatcher,
            [dispatcher, session]() {
                dispatcher->dispatchSessionStatusChanged(session);
            }, Qt::QueuedConnection);
    }

    FlashOperationView flashStatus() const
    {
        return m_flashOperation;
    }

    void loadFirmwarePackage(const std::string& path)
    {
        if (!m_runtime) return;

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();
        auto packagePath = path;

        auto future = QtConcurrent::run([rt, packagePath]() -> bool {
            auto result = rt->loadFirmwarePackage(packagePath);
            return result.isOk();
        });

        auto *watcher = new QFutureWatcher<bool>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher, packagePath]() {
                bool success = watcher->result();
                watcher->deleteLater();

                QMetaObject::invokeMethod(self, [self, success, packagePath]() {
                    if (success && self->m_impl->m_runtime) {
                        auto *pkg = self->m_impl->m_runtime->loadedPackage();
                        if (pkg) {
                            const auto& meta = pkg->metadata();
                            self->m_impl->m_flashOperation.packagePath = packagePath;
                            self->m_impl->m_flashOperation.packageName = meta.vendor + " " + meta.platform;
                            self->m_impl->m_flashOperation.vendor = meta.vendor;
                            self->m_impl->m_flashOperation.version = meta.version.toString();
                            self->m_impl->m_flashOperation.packageSize = meta.packageSize;
                            self->m_impl->m_flashOperation.imageCount = static_cast<uint32_t>(pkg->imageCount());
                            self->m_impl->m_flashOperation.status = FlashStatus::Idle;
                            self->m_impl->m_flashOperation.canStart = self->m_impl->m_runtime->isConnected();
                        }
                    } else {
                        self->m_impl->m_flashOperation = FlashOperationView{};
                    }
                }, Qt::QueuedConnection);

                auto *d = self->m_impl->m_dispatcher;
                QMetaObject::invokeMethod(d,
                    [d, success, packagePath]() {
                        d->dispatchFlashOperationChanged({});
                    }, Qt::QueuedConnection);

                QMetaObject::invokeMethod(self, [self, success, packagePath]() {
                    emit self->packageLoaded(
                        QString::fromStdString(packagePath), success);
                }, Qt::QueuedConnection);

                self->m_impl->updateFlashOperationFromRuntime();
                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void validateFirmware()
    {
        if (!m_runtime) return;

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();

        auto future = QtConcurrent::run([rt]() -> bool {
            auto *pkg = rt->loadedPackage();
            if (!pkg) return false;
            auto result = pkg->validateBasic();
            return result.valid;
        });

        auto *watcher = new QFutureWatcher<bool>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher]() {
                bool valid = watcher->result();
                watcher->deleteLater();

                self->m_impl->m_flashOperation.canStart = valid && self->m_impl->m_runtime && self->m_impl->m_runtime->isConnected();
                self->m_impl->updateFlashOperationFromRuntime();

                QMetaObject::invokeMethod(self, [self, valid]() {
                    emit self->firmwareValidated(valid,
                        valid ? "Firmware validation passed" : "Firmware validation failed");
                }, Qt::QueuedConnection);

                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void startFlash()
    {
        if (!m_runtime || m_flashOperation.status == FlashStatus::Flashing) return;

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();
        m_flashOperation.status = FlashStatus::Preparing;
        m_flashProgress = FlashProgressView{};

        QMetaObject::invokeMethod(dispatcher,
            [dispatcher, op = m_flashOperation]() {
                dispatcher->dispatchFlashOperationChanged(op);
            }, Qt::QueuedConnection);

        auto errorPtr = std::make_shared<std::string>();
        auto future = QtConcurrent::run([rt, self, errorPtr]() -> bool {
            self->m_impl->m_flashOperation.status = FlashStatus::Flashing;
            self->m_impl->m_flashProgress.status = FlashStatus::Flashing;
            self->m_impl->m_flashProgress.stage = FlashStage::Programming;

            auto *d = self->m_impl->m_dispatcher;
            QMetaObject::invokeMethod(d,
                [d, progress = self->m_impl->m_flashProgress]() {
                    d->dispatchFlashProgressChanged(progress);
                }, Qt::QueuedConnection);

            auto pkgPath = self->m_impl->m_flashOperation.packagePath;
            auto result = rt->flash(pkgPath);

            if (!result.isOk()) {
                *errorPtr = RuntimeErrorMapper::map(result.error()).message;
            }
            return result.isOk();
        });

        auto *watcher = new QFutureWatcher<bool>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher, errorPtr]() {
                bool success = watcher->result();
                watcher->deleteLater();

                FlashResultView flashResult;
                flashResult.packageName = self->m_impl->m_flashOperation.packageName;
                flashResult.deviceName = self->m_impl->m_flashOperation.vendor;
                flashResult.bytesWritten = self->m_impl->m_flashProgress.transferredBytes;

                if (success) {
                    self->m_impl->m_flashOperation.status = FlashStatus::Completed;
                    self->m_impl->m_flashProgress.status = FlashStatus::Completed;
                    self->m_impl->m_flashProgress.percentage = 100.0;
                    flashResult.success = true;
                    flashResult.finalStatus = FlashStatus::Completed;

                    QMetaObject::invokeMethod(self, [self]() {
                        emit self->flashCompleted(FlashResultView{});
                    }, Qt::QueuedConnection);
                } else {
                    self->m_impl->m_flashOperation.status = FlashStatus::Failed;
                    self->m_impl->m_flashProgress.status = FlashStatus::Failed;

                    flashResult.success = false;
                    flashResult.finalStatus = FlashStatus::Failed;
                    flashResult.errorMessage = *errorPtr;
                    flashResult.recoveryRecommendation = "Check connection and firmware compatibility";

                    QMetaObject::invokeMethod(self, [self, msg = flashResult.errorMessage]() {
                        emit self->flashFailed(QString::fromStdString(msg));
                    }, Qt::QueuedConnection);
                }

                self->m_impl->m_flashOperation.canStart = false;
                self->m_impl->m_flashOperation.canCancel = false;
                self->m_impl->updateFlashOperationFromRuntime();

                auto *d = self->m_impl->m_dispatcher;
                QMetaObject::invokeMethod(d,
                    [d, progress = self->m_impl->m_flashProgress]() {
                        d->dispatchFlashProgressChanged(progress);
                    }, Qt::QueuedConnection);
                QMetaObject::invokeMethod(d,
                    [d, result = flashResult]() {
                        d->dispatchFlashResult(result);
                    }, Qt::QueuedConnection);

                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void cancelFlash()
    {
        if (!m_runtime) return;
        if (m_flashOperation.status != FlashStatus::Flashing &&
            m_flashOperation.status != FlashStatus::Preparing &&
            m_flashOperation.status != FlashStatus::Validating) {
            return;
        }

        ++m_pendingAsyncTasks;
        auto *self = m_owner;
        auto *dispatcher = m_dispatcher;
        auto *rt = m_runtime.get();

        auto future = QtConcurrent::run([rt]() {
            rt->cancel();
        });

        auto *watcher = new QFutureWatcher<void>(self);
        QObject::connect(watcher, &QFutureWatcherBase::finished, self,
            [self, dispatcher, watcher]() {
                watcher->deleteLater();

                self->m_impl->m_flashOperation.status = FlashStatus::Cancelled;
                self->m_impl->m_flashOperation.canStart = true;
                self->m_impl->m_flashOperation.canCancel = false;
                self->m_impl->m_flashProgress.status = FlashStatus::Cancelled;
                self->m_impl->updateFlashOperationFromRuntime();

                auto *d = self->m_impl->m_dispatcher;
                QMetaObject::invokeMethod(d,
                    [d, progress = self->m_impl->m_flashProgress]() {
                        d->dispatchFlashProgressChanged(progress);
                    }, Qt::QueuedConnection);

                QMetaObject::invokeMethod(self, [self]() {
                    emit self->flashCancelled();
                }, Qt::QueuedConnection);

                --self->m_impl->m_pendingAsyncTasks;
            });
        watcher->setFuture(future);
    }

    void updateFlashOperationFromRuntime()
    {
        auto *dispatcher = m_dispatcher;
        QMetaObject::invokeMethod(dispatcher,
            [dispatcher, op = m_flashOperation]() {
                dispatcher->dispatchFlashOperationChanged(op);
            }, Qt::QueuedConnection);
    }

protected:
    void onRuntimeEvent(const mbootcore::runtime::RuntimeEvent& event) override
    {
        QMetaObject::invokeMethod(m_dispatcher, [this, event]() {
            m_dispatcher->dispatchRuntimeEvent(event);
        }, Qt::QueuedConnection);
    }

    void onStatisticsUpdated(const mbootcore::runtime::RuntimeStatistics& stats) override
    {
        auto health = m_runtime->health();
        auto version = m_runtime->version();
        auto state = m_state;
        QMetaObject::invokeMethod(m_dispatcher,
            [this, stats, health, version, state]() {
                m_dispatcher->dispatchStatisticsUpdate(stats, health, version, state);
            }, Qt::QueuedConnection);
    }

    void onHealthChanged(const mbootcore::runtime::RuntimeHealth& health) override
    {
        auto stats = m_runtime->statistics();
        auto version = m_runtime->version();
        auto state = m_state;
        QMetaObject::invokeMethod(m_dispatcher,
            [this, stats, health, version, state]() {
                m_dispatcher->dispatchStatisticsUpdate(stats, health, version, state);
            }, Qt::QueuedConnection);
    }

private:
    void unregisterObserver()
    {
        if (m_runtime) {
            m_runtime->removeObserver(this);
        }
    }

    RuntimeBridge* m_owner;
    std::unique_ptr<mbootcore::runtime::Runtime> m_runtime;
    RuntimeEventDispatcher* m_dispatcher;
    RuntimeBridgeState m_state{RuntimeBridgeState::Uninitialized};
    std::atomic<int> m_pendingAsyncTasks{0};
    std::vector<mbootcore::discovery::DeviceDescriptor> m_discoveredDescriptors;
    std::string m_loadedPackagePath;
    FlashOperationView m_flashOperation;
    FlashProgressView m_flashProgress;
};

RuntimeBridge::RuntimeBridge(QObject *parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>(this))
{
}

RuntimeBridge::~RuntimeBridge()
{
    shutdown();
}

RuntimeBridge::RuntimeBridge(RuntimeBridge&& other) noexcept
    : QObject(nullptr)
    , m_impl(std::move(other.m_impl))
{
}

RuntimeBridge& RuntimeBridge::operator=(RuntimeBridge&& other) noexcept
{
    if (this != &other) {
        shutdown();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

mbootcore::runtime::Runtime* RuntimeBridge::runtime() const noexcept
{
    return m_impl->runtime();
}

bool RuntimeBridge::isInitialized() const noexcept
{
    return m_impl->state() == RuntimeBridgeState::Ready;
}

RuntimeBridgeState RuntimeBridge::state() const noexcept
{
    return m_impl->state();
}

mbootcore::Result<void> RuntimeBridge::initialize()
{
    return m_impl->initialize();
}

void RuntimeBridge::shutdown() noexcept
{
    m_impl->shutdown();
}

RuntimeEventDispatcher* RuntimeBridge::eventDispatcher() const noexcept
{
    return m_impl->dispatcher();
}

RuntimeStatusView RuntimeBridge::currentStatus() const
{
    return m_impl->currentStatus();
}

SessionInfoView RuntimeBridge::currentSessionStatus() const
{
    return m_impl->sessionStatus();
}

void RuntimeBridge::discoverDevices()
{
    m_impl->discoverDevices();
}

void RuntimeBridge::connectDevice(const std::string& connectionPath)
{
    m_impl->connectDevice(connectionPath);
}

void RuntimeBridge::disconnectDevice()
{
    m_impl->disconnectDevice();
}

void RuntimeBridge::refreshDeviceList()
{
    m_impl->updateDeviceListFromRuntime();
}

void RuntimeBridge::refreshSessionInfo()
{
    m_impl->updateSessionFromRuntime();
}

FlashOperationView RuntimeBridge::currentFlashStatus() const
{
    return m_impl->flashStatus();
}

void RuntimeBridge::loadFirmwarePackage(const std::string& path)
{
    m_impl->loadFirmwarePackage(path);
}

void RuntimeBridge::validateFirmware()
{
    m_impl->validateFirmware();
}

void RuntimeBridge::startFlash()
{
    m_impl->startFlash();
}

void RuntimeBridge::cancelFlash()
{
    m_impl->cancelFlash();
}

void RuntimeBridge::refreshFlashStatus()
{
    m_impl->updateFlashOperationFromRuntime();
}

RuntimeDiagnosticsView RuntimeBridge::runtimeDiagnostics() const
{
    if (!m_impl->runtime() || m_impl->state() == RuntimeBridgeState::Uninitialized) {
        return {};
    }
    auto *rt = m_impl->runtime();
    auto stats = rt->statistics();
    auto health = rt->health();
    auto hwReport = rt->hardwareReport();
    auto version = rt->version();
    return RuntimeConverters::fromCoreDiagnostics(stats, health, hwReport, version);
}

RuntimeHealthView RuntimeBridge::deviceDiagnostics() const
{
    if (!m_impl->runtime() || m_impl->state() == RuntimeBridgeState::Uninitialized) {
        return {};
    }
    auto health = m_impl->runtime()->health();
    return RuntimeConverters::fromCoreHealth(health);
}

void RuntimeBridge::refreshDiagnostics()
{
    if (!m_impl->runtime()) return;
    auto diag = runtimeDiagnostics();
    auto *ed = m_impl->dispatcher();
    QMetaObject::invokeMethod(ed,
        [ed, diag]() {
            ed->dispatchDiagnosticsUpdated(diag);
        }, Qt::QueuedConnection);
    auto health = deviceDiagnostics();
    QMetaObject::invokeMethod(ed,
        [ed, health]() {
            ed->dispatchHealthChanged(health);
        }, Qt::QueuedConnection);
}

std::vector<PluginInfoView> RuntimeBridge::enumeratePlugins() const
{
    if (!m_impl->runtime() || m_impl->state() == RuntimeBridgeState::Uninitialized) {
        return {};
    }
    auto names = m_impl->runtime()->listPlugins();
    std::vector<PluginInfoView> result;
    result.reserve(names.size());
    for (const auto& name : names) {
        result.push_back(RuntimeConverters::fromPluginName(name));
    }
    return result;
}

void RuntimeBridge::unloadPlugin(const std::string& name)
{
    if (!m_impl->runtime()) return;
    auto *rt = m_impl->runtime();
    auto pluginName = name;

    auto errorPtr = std::make_shared<std::string>();
    auto future = QtConcurrent::run([rt, pluginName, errorPtr]() -> bool {
        auto result = rt->removePlugin(pluginName);
        if (!result.isOk()) {
            *errorPtr = RuntimeErrorMapper::map(result.error()).message;
        }
        return result.isOk();
    });

    auto *watcher = new QFutureWatcher<bool>(this);
    QObject::connect(watcher, &QFutureWatcherBase::finished, this,
        [this, watcher, pluginName, errorPtr]() {
            bool success = watcher->result();
            watcher->deleteLater();
            auto *ed = m_impl->dispatcher();
            if (success) {
                QMetaObject::invokeMethod(ed,
                    [ed]() {
                        ed->dispatchPluginListChanged();
                    }, Qt::QueuedConnection);
                QMetaObject::invokeMethod(this,
                    [this, pluginName]() {
                        emit pluginUnloaded(QString::fromStdString(pluginName));
                    }, Qt::QueuedConnection);
            } else {
                auto error = RuntimeErrorMapper::map(mbootcore::ErrorCode::PluginNotFound);
                QMetaObject::invokeMethod(ed,
                    [ed, error]() {
                        ed->dispatchError(error);
                    }, Qt::QueuedConnection);
            }
        });
    watcher->setFuture(future);
}

PluginInfoView RuntimeBridge::pluginInformation(const std::string& name) const
{
    return RuntimeConverters::fromPluginName(name);
}

std::vector<CapabilityView> RuntimeBridge::pluginCapabilities() const
{
    return runtimeCapabilities();
}

void RuntimeBridge::refreshPluginList()
{
    auto *ed = m_impl->dispatcher();
    QMetaObject::invokeMethod(ed,
        [ed]() {
            ed->dispatchPluginListChanged();
        }, Qt::QueuedConnection);
}

std::vector<CapabilityView> RuntimeBridge::runtimeCapabilities() const
{
    if (!m_impl->runtime() || m_impl->state() == RuntimeBridgeState::Uninitialized) {
        return {};
    }
    auto caps = m_impl->runtime()->capabilities();
    std::vector<CapabilityView> result;
    result.reserve(caps.size());
    for (const auto& cap : caps) {
        result.push_back(RuntimeConverters::fromCapabilityString(cap));
    }
    return result;
}

} // namespace runtime
} // namespace gui
