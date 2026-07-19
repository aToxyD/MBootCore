#include <mbootcore/runtime/Runtime.hpp>

#include "DeviceService.hpp"
#include "FirmwareService.hpp"
#include "WorkflowService.hpp"
#include "PluginService.hpp"
#include "DiagnosticsService.hpp"
#include "RuntimeServices.hpp"
#include "RuntimeState.hpp"
#include "OperationContext.hpp"
#include "OperationHooks.hpp"

#include <mbootcore/logging/ConsoleLogger.hpp>
#include <mbootcore/plugin/PluginContext.hpp>



namespace mbootcore {
namespace runtime {

// ============================================================
// ObserverManager
// ============================================================

void ObserverManager::addObserver(RuntimeObserver* observer) {
    if (!observer) return;
    auto sp = std::shared_ptr<RuntimeObserver>(observer, [](RuntimeObserver*) {});
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& existing : m_observers) {
        if (existing.get() == observer) return;
    }
    m_observers.push_back(std::move(sp));
}

void ObserverManager::addObserver(std::shared_ptr<RuntimeObserver> observer) {
    if (!observer) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.push_back(std::move(observer));
}

void ObserverManager::removeObserver(RuntimeObserver* observer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_observers.begin(); it != m_observers.end(); ) {
        if (it->get() == observer) {
            it = m_observers.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<std::shared_ptr<RuntimeObserver>> ObserverManager::copyObservers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_observers;
}

void ObserverManager::notifyEvent(const RuntimeEvent& event) {
    auto observers = copyObservers();
    for (auto& obs : observers) obs->onRuntimeEvent(event);
}

void ObserverManager::notifyStatistics(const RuntimeStatistics& stats) {
    auto observers = copyObservers();
    for (auto& obs : observers) obs->onStatisticsUpdated(stats);
}

void ObserverManager::notifyHealth(const RuntimeHealth& health) {
    auto observers = copyObservers();
    for (auto& obs : observers) obs->onHealthChanged(health);
}

// ============================================================
// Orchestrate — composition of OperationContext + OperationHooks + notifyHealth
// ============================================================

template<typename Fn>
auto Runtime::orchestrate(Operation op, Fn&& fn, OperationOptions options)
{
    auto result = OperationContext(*m_state, m_state->opMutex)
        .invoke(std::forward<Fn>(fn), options.lockMode);

    if (result.isOk()) {
        m_hooks->onSuccess(op);
    } else if (result.error() == ErrorCode::Cancelled) {
        m_hooks->onCancel(op);
    } else {
        m_hooks->onFailure(op, result.error());
    }

    notifyHealth();
    return result;
}

Runtime::Runtime()
    : m_state(std::make_unique<RuntimeState>())
    , m_observers(std::make_unique<ObserverManager>())
{
    m_hooks = std::make_unique<OperationHooks>(*m_state, *m_observers);
}

Runtime::~Runtime() {
    shutdown();
}

Runtime::Runtime(Runtime&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_state(std::move(other.m_state))
    , m_observers(std::move(other.m_observers))
    , m_hooks(std::make_unique<OperationHooks>(*m_state, *m_observers))
    , m_callbacks(std::move(other.m_callbacks))
    , m_logger(std::move(other.m_logger))
    , m_deviceService(std::move(other.m_deviceService))
    , m_firmwareService(std::move(other.m_firmwareService))
    , m_transportManager(std::move(other.m_transportManager))
    , m_workflowService(std::move(other.m_workflowService))
    , m_pluginService(std::move(other.m_pluginService))
    , m_diagnosticsService(std::move(other.m_diagnosticsService))
    , m_loaderOverride(std::move(other.m_loaderOverride))
    , m_workflowFactoryOverride(std::move(other.m_workflowFactoryOverride))
    , m_jobSchedulerOverride(std::move(other.m_jobSchedulerOverride))
    , m_bootPipelineOverride(std::move(other.m_bootPipelineOverride))
    , m_pluginManagerOverride(std::move(other.m_pluginManagerOverride))
    , m_vendorRuntimeOverride(std::move(other.m_vendorRuntimeOverride))
{
    // Rebuild m_services to reference our members, not other's moved-from members
    if (other.m_services) {
        m_services = std::make_unique<RuntimeServices>(
            *m_deviceService, *m_firmwareService, *m_workflowService,
            *m_pluginService, *m_diagnosticsService);
    }
}

Runtime& Runtime::operator=(Runtime&& other) noexcept {
    if (this != &other) {
        shutdown();
        m_state = std::move(other.m_state);
        m_observers = std::move(other.m_observers);
        m_hooks = std::make_unique<OperationHooks>(*m_state, *m_observers);
        m_callbacks = std::move(other.m_callbacks);
        m_config = std::move(other.m_config);
        m_logger = std::move(other.m_logger);
        m_deviceService = std::move(other.m_deviceService);
        m_firmwareService = std::move(other.m_firmwareService);
        m_transportManager = std::move(other.m_transportManager);
        m_workflowService = std::move(other.m_workflowService);
        m_pluginService = std::move(other.m_pluginService);
        m_diagnosticsService = std::move(other.m_diagnosticsService);
        m_loaderOverride = std::move(other.m_loaderOverride);
        m_workflowFactoryOverride = std::move(other.m_workflowFactoryOverride);
        m_jobSchedulerOverride = std::move(other.m_jobSchedulerOverride);
        m_bootPipelineOverride = std::move(other.m_bootPipelineOverride);
        m_pluginManagerOverride = std::move(other.m_pluginManagerOverride);
        m_vendorRuntimeOverride = std::move(other.m_vendorRuntimeOverride);
        // Rebuild m_services — it held references to other's members
        m_services = std::make_unique<RuntimeServices>(
            *m_deviceService, *m_firmwareService, *m_workflowService,
            *m_pluginService, *m_diagnosticsService);
    }
    return *this;
}

Result<void> Runtime::initialize() {
    {
        std::lock_guard<std::mutex> lock(m_state->opMutex);
        if (m_state->initialized) return ErrorCode::InvalidState;

        m_state->startTime = std::chrono::steady_clock::now();

        createDefaultComponents();

        if (!m_services) {
            m_services = std::make_unique<RuntimeServices>(*m_deviceService, *m_firmwareService, *m_workflowService, *m_pluginService, *m_diagnosticsService);
        }

        auto vr = m_pluginService->initialize();
        if (vr.isError()) return vr;

        m_state->initialized = true;
    }

    RuntimeEvent ev;
    ev.type = RuntimeEventType::RuntimeStarted;
    ev.message = "Runtime initialized";
    ev.timestamp = std::chrono::steady_clock::now();
    fireEvent(ev);

    if (m_callbacks.onLog) m_callbacks.onLog("Runtime initialized");
    if (m_callbacks.onStatus) m_callbacks.onStatus("Initialized");

    return {};
}

void Runtime::shutdown() noexcept {
    if (!m_state) return;
    {
        std::lock_guard<std::mutex> lock(m_state->opMutex);
        if (!m_state->initialized) return;
        m_state->initialized = false;
        m_state->connected = false;
        m_state->cancelled = false;
    }

    (void)m_pluginService->shutdown();

    RuntimeEvent ev;
    ev.type = RuntimeEventType::RuntimeStopped;
    ev.message = "Runtime shutdown";
    ev.timestamp = std::chrono::steady_clock::now();
    fireEvent(ev);

    if (m_callbacks.onLog) m_callbacks.onLog("Runtime shutdown");

    m_services.reset();
}

bool Runtime::isInitialized() const noexcept {
    return m_state->initialized.load();
}

// ============================================================
// Discovery
// ============================================================

Result<std::vector<discovery::DeviceDescriptor>> Runtime::discover(std::chrono::milliseconds timeout) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<std::vector<discovery::DeviceDescriptor>> {
            auto result = services().devices().discover(timeout);
            if (result.isOk()) {
                for (const auto& dev : result.value()) {
                    if (m_callbacks.onDeviceDiscovered) m_callbacks.onDeviceDiscovered(dev);
                }
            }
            return result;
        }, LockMode::None);
}

Result<discovery::DeviceDescriptor> Runtime::probe(const discovery::DeviceDescriptor& hint) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<discovery::DeviceDescriptor> {
            return services().devices().probe(hint);
        }, LockMode::None);
}

// ============================================================
// Connection
// ============================================================

Result<void> Runtime::connect(const discovery::DeviceDescriptor& descriptor) {
    return orchestrate(Operation::Connect, [&]() -> Result<void> {
        if (m_state->connected)
            return ErrorCode::SessionAlreadyConnected;

        auto result = services().devices().connect(descriptor);
        if (result.isError()) return result;

        m_state->connected = true;

        if (m_callbacks.onDeviceConnected) m_callbacks.onDeviceConnected(descriptor);
        if (m_callbacks.onStatus) m_callbacks.onStatus("Connected to " + descriptor.friendlyName);

        return {};
    }, OperationOptions{LockMode::Exclusive});
}

void Runtime::disconnect() {
    {
        std::lock_guard<std::mutex> lock(m_state->opMutex);
        if (!m_state->connected) return;
        services().devices().disconnect();
        m_state->connected = false;
    }

    m_hooks->onSuccess(Operation::Disconnect);
    notifyHealth();

    if (m_callbacks.onLog) m_callbacks.onLog("Device disconnected");
}

Result<void> Runtime::reconnect() {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().devices().reconnect();
        }, LockMode::Exclusive);
}

bool Runtime::isConnected() const noexcept {
    return m_state->connected.load();
}

session::DeviceSession* Runtime::activeSession() const noexcept {
    return m_deviceService ? m_deviceService->activeSession() : nullptr;
}

// ============================================================
// Flash operations
// ============================================================

Result<void> Runtime::flash(const std::string& packagePath) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().flash(packagePath);
        }, LockMode::None);
}

Result<void> Runtime::flash(firmware::FirmwarePackage& package) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            if (!m_state->connected)
                return ErrorCode::SessionNotConnected;
            return services().firmware().flash(package);
        }, LockMode::Exclusive);
}

Result<ByteBuffer> Runtime::read(uint64_t address, size_t size) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<ByteBuffer> {
            return services().firmware().read(address, size);
        }, LockMode::None);
}

Result<void> Runtime::write(uint64_t address, const ByteBuffer& data) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().write(address, data);
        }, LockMode::None);
}

Result<void> Runtime::erase(uint64_t address, size_t size) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().erase(address, size);
        }, LockMode::None);
}

Result<void> Runtime::verify(uint64_t address, const ByteBuffer& expected) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().verify(address, expected);
        }, LockMode::None);
}

// ============================================================
// Partition operations
// ============================================================

Result<ByteBuffer> Runtime::readPartition(const std::string& name) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<ByteBuffer> {
            return services().firmware().readPartition(name);
        }, LockMode::None);
}

Result<void> Runtime::writePartition(const std::string& name, const ByteBuffer& data) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().writePartition(name, data);
        }, LockMode::None);
}

Result<void> Runtime::erasePartition(const std::string& name) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().erasePartition(name);
        }, LockMode::None);
}

// ============================================================
// Backup / Restore
// ============================================================

Result<ByteBuffer> Runtime::backup(const std::string& partition) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<ByteBuffer> {
            return services().firmware().backup(partition);
        }, LockMode::None);
}

Result<void> Runtime::restore(const std::string& partition, const ByteBuffer& data) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return services().firmware().restore(partition, data);
        }, LockMode::None);
}

// ============================================================
// Workflow
// ============================================================

Result<void> Runtime::executeWorkflow(std::unique_ptr<workflow::IWorkflow> workflow) {
    return orchestrate(Operation::ExecuteWorkflow, [&]() -> Result<void> {
        RuntimeEvent startEv;
        startEv.type = RuntimeEventType::WorkflowStarted;
        startEv.message = "Workflow started";
        startEv.timestamp = std::chrono::steady_clock::now();
        m_observers->notifyEvent(startEv);

        auto wfStart = std::chrono::steady_clock::now();
        auto result = m_workflowService->executeWorkflow(std::move(workflow));
        auto wfElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - wfStart);

        {
            std::lock_guard<std::mutex> lock(m_state->statsMutex);
            m_state->stats.averageWorkflowTimeMs = m_state->stats.averageWorkflowTimeMs * 0.7 + static_cast<double>(wfElapsed.count()) * 0.3;
        }

        if (m_callbacks.onWorkflowProgress) {
            workflow::WorkflowProgress wp;
            wp.overallProgress = result.isOk() ? 100.0 : 0.0;
            m_callbacks.onWorkflowProgress(wp);
        }

        return result;
    });
}

Result<void> Runtime::executeWorkflow(const std::string& workflowType) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            if (!m_state->connected)
                return ErrorCode::SessionNotConnected;
            return m_workflowService->executeWorkflow(workflowType);
        }, LockMode::Exclusive);
}

// ============================================================
// Jobs
// ============================================================

Result<void> Runtime::runJob(std::unique_ptr<job::IJob> job) {
    return orchestrate(Operation::RunJob, [&]() -> Result<void> {
        auto result = m_workflowService->runJob(std::move(job));
        if (result.isError()) return result;

        if (m_callbacks.onStatus) m_callbacks.onStatus("Job scheduled");

        return {};
    });
}

Result<void> Runtime::runJobs(std::vector<std::unique_ptr<job::IJob>> jobs) {
    return orchestrate(Operation::RunJobs, [&]() -> Result<void> {
        auto result = m_workflowService->runJobs(std::move(jobs));
        if (result.isError()) return result;

        return {};
    });
}

// ============================================================
// Plugins
// ============================================================

Result<void> Runtime::installPlugin(std::unique_ptr<plugin::IPlugin> plugin) {
    return orchestrate(Operation::InstallPlugin, [&]() -> Result<void> {
        auto result = m_pluginService->installPlugin(std::move(plugin));
        if (result.isError()) return result;

        if (m_callbacks.onLog) m_callbacks.onLog("Plugin installed successfully");

        return {};
    });
}

Result<void> Runtime::removePlugin(const std::string& name) {
    return OperationContext(*m_state, m_state->opMutex)
        .invoke([&]() -> Result<void> {
            return m_pluginService->removePlugin(name);
        }, LockMode::None);
}

std::vector<std::string> Runtime::listPlugins() const {
    if (!m_state->initialized) return {};
    return m_pluginService->listPlugins();
}

// ============================================================
// Vendors
// ============================================================

Result<void> Runtime::registerVendor(std::unique_ptr<vendor::IVendor> vendor) {
    return orchestrate(Operation::RegisterVendor, [&]() -> Result<void> {
        auto result = m_pluginService->registerVendor(std::move(vendor));
        if (result.isError()) return result;

        return {};
    });
}

// ============================================================
// Firmware
// ============================================================

Result<std::unique_ptr<firmware::FirmwarePackage>> Runtime::loadFirmwarePackage(const std::string& path) {
    return orchestrate(Operation::LoadFirmwarePackage, [&]() -> Result<std::unique_ptr<firmware::FirmwarePackage>> {
        auto result = services().firmware().loadFirmwarePackage(path);
        if (result.isError()) return result;

        if (m_callbacks.onStatus) m_callbacks.onStatus("Package loaded: " + path);

        return result;
    });
}

firmware::FirmwarePackage* Runtime::loadedPackage() const noexcept {
    return m_firmwareService ? m_firmwareService->loadedPackage() : nullptr;
}

// ============================================================
// Observers
// ============================================================

void Runtime::addObserver(RuntimeObserver* observer) {
    m_observers->addObserver(observer);
}

void Runtime::addObserver(std::shared_ptr<RuntimeObserver> observer) {
    m_observers->addObserver(std::move(observer));
}

void Runtime::removeObserver(RuntimeObserver* observer) {
    m_observers->removeObserver(observer);
}

// ============================================================
// Callbacks
// ============================================================

void Runtime::setCallbacks(const RuntimeCallbacks& callbacks) {
    m_callbacks = callbacks;
}

RuntimeCallbacks& Runtime::callbacks() noexcept {
    return m_callbacks;
}

// ============================================================
// Statistics
// ============================================================

RuntimeStatistics Runtime::statistics() const {
    return m_diagnosticsService ? m_diagnosticsService->statistics() : RuntimeStatistics{};
}

RuntimeHealth Runtime::health() const {
    return m_diagnosticsService ? m_diagnosticsService->health() : RuntimeHealth{};
}

// ============================================================
// Control
// ============================================================

void Runtime::cancel() {
    if (!m_state) return;
    m_state->cancelled = true;
    if (m_deviceService) {
        auto* session = m_deviceService->activeSession();
        if (session) session->cancel();
    }
    if (m_workflowService) m_workflowService->cancel();
    if (m_callbacks.onLog) m_callbacks.onLog("Runtime cancelled");
}

void Runtime::pause() {
    if (!m_state) return;
    m_state->paused = true;
    if (m_workflowService) m_workflowService->pause();
    if (m_callbacks.onLog) m_callbacks.onLog("Runtime paused");
}

void Runtime::resume() {
    if (!m_state) return;
    m_state->paused = false;
    if (m_workflowService) m_workflowService->resume();
    if (m_callbacks.onLog) m_callbacks.onLog("Runtime resumed");
}

Result<void> Runtime::reset() {
    m_state->cancelled = false;
    m_state->paused = false;

    disconnect();

    if (m_workflowService) m_workflowService->reset();

    if (m_callbacks.onLog) m_callbacks.onLog("Runtime reset");

    return {};
}

// ============================================================
// Version & Capabilities
// ============================================================

std::string Runtime::version() const {
    return m_diagnosticsService ? m_diagnosticsService->versionString() : "MBootCore " MBOOTCORE_VERSION;
}

std::vector<std::string> Runtime::capabilities() const {
    std::vector<std::string> caps = {
        "discovery",
        "sahara",
        "firehose",
        "gpt",
        "flash",
        "backup",
        "restore",
        "verify",
        "workflow",
        "jobs",
        "plugins",
        "vendors",
        "elf",
        "loader",
        "pipeline",
        "transport",
        "monitoring",
        "recovery",
        "hardware"
    };
#ifdef MBOOTCORE_HAVE_LIBUSB
    caps.push_back("usb-backend");
    caps.push_back("libusb");
#endif
#ifdef _WIN32
    caps.push_back("usb-backend");
    caps.push_back("winusb");
#endif
    return caps;
}

// ============================================================
// Hardware Integration
// ============================================================

HardwareDiagnosticReport Runtime::hardwareReport() const {
    return m_diagnosticsService ? m_diagnosticsService->hardwareReport() : HardwareDiagnosticReport{};
}

std::vector<HardwareDeviceEntry> Runtime::connectedHardware() const {
    return m_diagnosticsService ? m_diagnosticsService->connectedHardware() : std::vector<HardwareDeviceEntry>{};
}

HardwareTransportStats Runtime::transportStatistics(transport::TransportType type) const {
    return m_diagnosticsService ? m_diagnosticsService->transportStatistics(type) : HardwareTransportStats{};
}

std::vector<transport::usb::UsbDeviceInfo> Runtime::usbDevices() const {
    return m_diagnosticsService ? m_diagnosticsService->usbDevices() : std::vector<transport::usb::UsbDeviceInfo>{};
}

std::vector<std::string> Runtime::serialPorts() const {
    return m_diagnosticsService ? m_diagnosticsService->serialPorts() : std::vector<std::string>{};
}

transport::TransportManager& Runtime::transportManager() noexcept { return *m_transportManager; }


Services& Runtime::services() noexcept {
    return *m_services;
}

void Runtime::createDefaultComponents() {
    if (!m_logger) {
        m_logger = std::make_unique<ConsoleLogger>();
        m_logger->setLevel(LogLevel::Info);
    }

    if (!m_deviceService) {
        auto registry = std::make_unique<discovery::ProtocolRegistry>();
        auto* regPtr = registry.get();
        m_deviceService = std::make_unique<DeviceService>(
            std::move(registry),
            std::make_unique<discovery::DeviceDiscoveryEngine>(*regPtr),
            std::make_unique<discovery::ProtocolNegotiationEngine>(*regPtr),
            std::make_unique<session::DeviceManager>(*regPtr));
    }

    auto& protocolRegistry = m_deviceService->protocolRegistry();
    auto* sessionManager = &m_deviceService->sessionManager();

    if (!m_transportManager) {
        m_transportManager = std::make_unique<transport::TransportManager>();
        m_transportManager->setCallback([this](const std::string& id, transport::TransportState state) {
            if (state == transport::TransportState::Error) {
                RuntimeEvent ev;
                ev.type = RuntimeEventType::TransportLost;
                ev.source = id;
                ev.message = "Transport error: " + id;
                ev.timestamp = std::chrono::steady_clock::now();
                fireEvent(ev);
                ++m_state->stats.totalErrors;
            }
        });
    }

    if (!m_pluginService) {
        auto vendorRt = m_vendorRuntimeOverride
            ? std::move(m_vendorRuntimeOverride)
            : std::make_unique<vendor::VendorRuntime>();

        auto pCtx = std::make_unique<plugin::PluginContext>(
            protocolRegistry, sessionManager, m_logger.get());

        auto pMgr = m_pluginManagerOverride
            ? std::move(m_pluginManagerOverride)
            : std::make_unique<plugin::PluginManager>(*pCtx);

        m_pluginService = std::make_unique<PluginService>(
            std::move(pMgr),
            std::move(pCtx),
            std::move(vendorRt),
            *m_logger);
    }

    // WorkflowFactory: created first to break circular dependency between
    // FirmwareService (needs WorkflowFactory&) and WorkflowService (needs IFirmwareService&)
    auto workflowFactory = m_workflowFactoryOverride
        ? std::move(m_workflowFactoryOverride)
        : std::make_unique<workflow::WorkflowFactory>();

    if (!m_firmwareService) {
        auto validator = std::make_unique<firmware::FirmwareValidator>();
        validator->setLogger(m_logger.get());
        auto resolver = std::make_unique<firmware::FirmwareResolver>();
        auto imgEngine = std::make_unique<firmware::ImageEngine>();
        auto fpg = std::make_unique<firmware::FlashPlanGenerator>();
        auto executor = std::make_unique<firmware::FirmwareExecutor>();

        auto loaderFw = m_loaderOverride
            ? std::move(m_loaderOverride)
            : std::make_unique<LoaderFramework>(
                std::shared_ptr<ILogger>(m_logger.get(), [](ILogger*){}));

        m_firmwareService = std::make_unique<FirmwareService>(
            std::move(validator),
            std::move(resolver),
            std::move(imgEngine),
            std::move(fpg),
            std::move(executor),
            std::move(loaderFw),
            *m_logger,
            *m_deviceService,
            *workflowFactory,
            [this](std::unique_ptr<workflow::IWorkflow> wf) {
                return this->executeWorkflow(std::move(wf));
            });
    }

    if (!m_workflowService) {
        auto jobCtx = std::make_unique<job::JobContext>();
        jobCtx->logger = m_logger.get();

        auto scheduler = m_jobSchedulerOverride
            ? std::move(m_jobSchedulerOverride)
            : std::make_unique<job::JobScheduler>(*jobCtx);

        auto pipeline = m_bootPipelineOverride
            ? std::move(m_bootPipelineOverride)
            : std::make_unique<pipeline::BootPipeline>();

        m_workflowService = std::make_unique<WorkflowService>(
            std::move(workflowFactory),
            std::move(jobCtx),
            std::move(scheduler),
            std::move(pipeline),
            *m_logger,
            *m_deviceService,
            *m_firmwareService);
    }

    if (!m_diagnosticsService) {
        m_diagnosticsService = std::make_unique<DiagnosticsService>(
            *m_state, *m_transportManager, *m_deviceService, *m_pluginService);
    }
}

void Runtime::fireEvent(const RuntimeEvent& event) {
    m_observers->notifyEvent(event);
    updateStatistics();
}

void Runtime::updateStatistics() {
    RuntimeStatistics snapshot;
    {
        std::lock_guard<std::mutex> lock(m_state->statsMutex);
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::steady_clock::now() - m_state->startTime);
        m_state->stats.uptimeSeconds = elapsed.count();
        snapshot = m_state->stats;
    }
    m_observers->notifyStatistics(snapshot);
    auto h = health();
    m_observers->notifyHealth(h);
}

void Runtime::notifyHealth() {
    m_observers->notifyHealth(health());
}

} // namespace runtime
} // namespace mbootcore
