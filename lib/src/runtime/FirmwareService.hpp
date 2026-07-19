#pragma once

#include <mbootcore/runtime/IFirmwareService.hpp>
#include <mbootcore/runtime/IDeviceService.hpp>

#include <mbootcore/firmware/FirmwareValidator.hpp>
#include <mbootcore/firmware/FirmwareResolver.hpp>
#include <mbootcore/firmware/ImageEngine.hpp>
#include <mbootcore/firmware/FlashPlan.hpp>
#include <mbootcore/firmware/FirmwareExecutor.hpp>
#include <mbootcore/loader/LoaderFramework.hpp>
#include <mbootcore/workflow/WorkflowFactory.hpp>
#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/domain/ILogger.hpp>

#include <memory>
#include <functional>

namespace mbootcore {
namespace runtime {

class FirmwareService final : public IFirmwareService {
public:
    using WorkflowExecutor = std::function<Result<void>(std::unique_ptr<workflow::IWorkflow>)>;

    FirmwareService(
        std::unique_ptr<firmware::FirmwareValidator> validator,
        std::unique_ptr<firmware::FirmwareResolver> resolver,
        std::unique_ptr<firmware::ImageEngine> imageEngine,
        std::unique_ptr<firmware::FlashPlanGenerator> flashPlanGenerator,
        std::unique_ptr<firmware::FirmwareExecutor> executor,
        std::unique_ptr<LoaderFramework> loaderFramework,
        ILogger& logger,
        IDeviceService& deviceService,
        workflow::WorkflowFactory& workflowFactory,
        WorkflowExecutor workflowExec);

    ~FirmwareService() override = default;

    FirmwareService(const FirmwareService&) = delete;
    FirmwareService& operator=(const FirmwareService&) = delete;
    FirmwareService(FirmwareService&&) noexcept = delete;
    FirmwareService& operator=(FirmwareService&&) noexcept = delete;

    // Subsystem accessors (needed by RuntimeBuilder)
    firmware::FirmwareValidator& firmwareValidator() noexcept { return *m_firmwareValidator; }
    firmware::FirmwareResolver& firmwareResolver() noexcept { return *m_firmwareResolver; }
    firmware::ImageEngine& imageEngine() noexcept { return *m_imageEngine; }
    firmware::FlashPlanGenerator& flashPlanGenerator() noexcept { return *m_flashPlanGenerator; }
    firmware::FirmwareExecutor& firmwareExecutor() noexcept { return *m_firmwareExecutor; }
    LoaderFramework& loaderFramework() noexcept { return *m_loaderFramework; }

    // IFirmwareService
    Result<void> flash(const std::string& packagePath) override;
    Result<void> flash(firmware::FirmwarePackage& package) override;
    Result<ByteBuffer> read(uint64_t address, size_t size) override;
    Result<void> write(uint64_t address, const ByteBuffer& data) override;
    Result<void> erase(uint64_t address, size_t size) override;
    Result<void> verify(uint64_t address, const ByteBuffer& expected) override;
    Result<ByteBuffer> readPartition(const std::string& name) override;
    Result<void> writePartition(const std::string& name, const ByteBuffer& data) override;
    Result<void> erasePartition(const std::string& name) override;
    Result<ByteBuffer> backup(const std::string& partition) override;
    Result<void> restore(const std::string& partition, const ByteBuffer& data) override;
    Result<std::unique_ptr<firmware::FirmwarePackage>> loadFirmwarePackage(
        const std::string& path) override;
    firmware::FirmwarePackage* loadedPackage() const noexcept override;

private:
    session::DeviceSession* activeSession() const noexcept;

    std::unique_ptr<firmware::FirmwareValidator> m_firmwareValidator;
    std::unique_ptr<firmware::FirmwareResolver> m_firmwareResolver;
    std::unique_ptr<firmware::ImageEngine> m_imageEngine;
    std::unique_ptr<firmware::FlashPlanGenerator> m_flashPlanGenerator;
    std::unique_ptr<firmware::FirmwareExecutor> m_firmwareExecutor;
    std::unique_ptr<LoaderFramework> m_loaderFramework;
    std::unique_ptr<firmware::FirmwarePackage> m_loadedPackage;
    firmware::FirmwarePackage* m_loadedPackageRaw{nullptr};
    ILogger& m_logger;
    IDeviceService& m_deviceService;
    workflow::WorkflowFactory& m_workflowFactory;
    WorkflowExecutor m_workflowExec;
};

} // namespace runtime
} // namespace mbootcore
