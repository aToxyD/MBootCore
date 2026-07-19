#pragma once

#include <mbootcore/runtime/IRuntime.hpp>
#include <mbootcore/runtime/OrchestrationTypes.hpp>
#include <mbootcore/runtime/RuntimeConfig.hpp>
#include <mbootcore/runtime/RuntimeEvents.hpp>
#include <mbootcore/runtime/RuntimeCallbacks.hpp>
#include <mbootcore/runtime/RuntimeObserver.hpp>
#include <mbootcore/runtime/RuntimeStatistics.hpp>
#include <mbootcore/runtime/RuntimeHardware.hpp>
#include <mbootcore/transport/usb/UsbDeviceInfo.hpp>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/domain/Types.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/transport/TransportManager.hpp>
#include <mbootcore/session/DeviceManager.hpp>
#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/session/SessionTypes.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareTypes.hpp>

#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

// Forward declarations
namespace mbootcore::workflow { class IWorkflow; class WorkflowFactory; }
namespace mbootcore::job { class IJob; class JobScheduler; }
namespace mbootcore::pipeline { class BootPipeline; }
namespace mbootcore::vendor { class IVendor; class VendorRuntime; }
namespace mbootcore::plugin { class IPlugin; class PluginManager; }
namespace mbootcore { class LoaderFramework; }

namespace mbootcore {
namespace runtime {

class RuntimeBuilder;

struct RuntimeState;
class ObserverManager;
class DeviceService;
class FirmwareService;
class WorkflowService;
class PluginService;
class DiagnosticsService;
class OperationHooks;

class Runtime final : public IRuntime {
    friend class RuntimeBuilder;
    friend class RuntimeFactory;

public:
    Runtime();
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) noexcept;
    Runtime& operator=(Runtime&&) noexcept;

    // Lifecycle
    Result<void> initialize() override;
    void shutdown() noexcept override;
    bool isInitialized() const noexcept;

    // Discovery
    Result<std::vector<discovery::DeviceDescriptor>> discover(
        std::chrono::milliseconds timeout = std::chrono::seconds(5));
    Result<discovery::DeviceDescriptor> probe(const discovery::DeviceDescriptor& hint);

    // Connection
    Result<void> connect(const discovery::DeviceDescriptor& descriptor);
    void disconnect();
    Result<void> reconnect();
    bool isConnected() const noexcept;
    session::DeviceSession* activeSession() const noexcept;

    // Flash operations
    Result<void> flash(const std::string& packagePath);
    Result<void> flash(firmware::FirmwarePackage& package);
    Result<ByteBuffer> read(uint64_t address, size_t size);
    Result<void> write(uint64_t address, const ByteBuffer& data);
    Result<void> erase(uint64_t address, size_t size);
    Result<void> verify(uint64_t address, const ByteBuffer& expected);

    // Partition operations
    Result<ByteBuffer> readPartition(const std::string& name);
    Result<void> writePartition(const std::string& name, const ByteBuffer& data);
    Result<void> erasePartition(const std::string& name);

    // Backup / Restore
    Result<ByteBuffer> backup(const std::string& partition);
    Result<void> restore(const std::string& partition, const ByteBuffer& data);

    // Workflow
    Result<void> executeWorkflow(std::unique_ptr<workflow::IWorkflow> workflow);
    Result<void> executeWorkflow(const std::string& workflowType);

    // Jobs
    Result<void> runJob(std::unique_ptr<job::IJob> job);
    Result<void> runJobs(std::vector<std::unique_ptr<job::IJob>> jobs);

    // Plugins
    Result<void> installPlugin(std::unique_ptr<plugin::IPlugin> plugin);
    Result<void> removePlugin(const std::string& name);
    std::vector<std::string> listPlugins() const;

    // Vendors
    Result<void> registerVendor(std::unique_ptr<vendor::IVendor> vendor);

    // Firmware
    Result<std::unique_ptr<firmware::FirmwarePackage>> loadFirmwarePackage(const std::string& path);
    firmware::FirmwarePackage* loadedPackage() const noexcept;

    // Observers
    void addObserver(RuntimeObserver* observer) override;
    void addObserver(std::shared_ptr<RuntimeObserver> observer);
    void removeObserver(RuntimeObserver* observer) override;

    // Callbacks
    void setCallbacks(const RuntimeCallbacks& callbacks) override;
    RuntimeCallbacks& callbacks() noexcept override;

    // Statistics
    RuntimeStatistics statistics() const override;
    RuntimeHealth health() const override;

    // Control
    void cancel();
    void pause() override;
    void resume() override;
    Result<void> reset() override;

    // Version & Capabilities
    std::string version() const override;
    std::vector<std::string> capabilities() const override;

    // Hardware Integration
    HardwareDiagnosticReport hardwareReport() const;
    std::vector<HardwareDeviceEntry> connectedHardware() const;
    HardwareTransportStats transportStatistics(transport::TransportType type) const;
    std::vector<transport::usb::UsbDeviceInfo> usbDevices() const;
    std::vector<std::string> serialPorts() const;

    // Transport access (only remaining subsystem directly owned by Runtime)
    transport::TransportManager& transportManager() noexcept;

    // Services accessor
    Services& services() noexcept override;

private:
    void createDefaultComponents();
    void fireEvent(const RuntimeEvent& event);
    void updateStatistics();
    void notifyHealth();

    template<typename Fn>
    auto orchestrate(Operation op, Fn&& fn, OperationOptions options = {});

    RuntimeConfig m_config;

    // Heap state (move-safe — contains mutexes)
    std::unique_ptr<RuntimeState> m_state;

    // Heap observers (move-safe — contains mutex)
    std::unique_ptr<ObserverManager> m_observers;

    // Operation hooks — side effect handler (events + stats)
    std::unique_ptr<OperationHooks> m_hooks;

    // Callbacks (movable)
    RuntimeCallbacks m_callbacks;

    std::unique_ptr<ILogger> m_logger;

    // Owned components (all unique_ptr, move-safe)
    std::unique_ptr<DeviceService> m_deviceService;
    std::unique_ptr<FirmwareService> m_firmwareService;
    std::unique_ptr<transport::TransportManager> m_transportManager;

    // Built-in services
    std::unique_ptr<WorkflowService> m_workflowService;
    std::unique_ptr<PluginService> m_pluginService;
    std::unique_ptr<DiagnosticsService> m_diagnosticsService;

    // Builder override holders (consumed by createDefaultComponents)
    std::unique_ptr<LoaderFramework> m_loaderOverride;
    std::unique_ptr<workflow::WorkflowFactory> m_workflowFactoryOverride;
    std::unique_ptr<job::JobScheduler> m_jobSchedulerOverride;
    std::unique_ptr<pipeline::BootPipeline> m_bootPipelineOverride;
    std::unique_ptr<plugin::PluginManager> m_pluginManagerOverride;
    std::unique_ptr<vendor::VendorRuntime> m_vendorRuntimeOverride;

    std::unique_ptr<Services> m_services;
};

} // namespace runtime
} // namespace mbootcore
