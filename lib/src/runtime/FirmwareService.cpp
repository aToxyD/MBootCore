#include "FirmwareService.hpp"

namespace mbootcore {
namespace runtime {

FirmwareService::FirmwareService(
    std::unique_ptr<firmware::FirmwareValidator> validator,
    std::unique_ptr<firmware::FirmwareResolver> resolver,
    std::unique_ptr<firmware::ImageEngine> imageEngine,
    std::unique_ptr<firmware::FlashPlanGenerator> flashPlanGenerator,
    std::unique_ptr<firmware::FirmwareExecutor> executor,
    std::unique_ptr<LoaderFramework> loaderFramework,
    ILogger& logger,
    IDeviceService& deviceService,
    workflow::WorkflowFactory& workflowFactory,
    WorkflowExecutor workflowExec)
    : m_firmwareValidator(std::move(validator))
    , m_firmwareResolver(std::move(resolver))
    , m_imageEngine(std::move(imageEngine))
    , m_flashPlanGenerator(std::move(flashPlanGenerator))
    , m_firmwareExecutor(std::move(executor))
    , m_loaderFramework(std::move(loaderFramework))
    , m_logger(logger)
    , m_deviceService(deviceService)
    , m_workflowFactory(workflowFactory)
    , m_workflowExec(std::move(workflowExec))
{
}

session::DeviceSession* FirmwareService::activeSession() const noexcept {
    return m_deviceService.activeSession();
}

Result<std::unique_ptr<firmware::FirmwarePackage>>
FirmwareService::loadFirmwarePackage(const std::string& path) {
    MBOOT_TRY_ASSIGN(resolved, m_firmwareResolver->resolve(path));

    if (!resolved.package)
        return ErrorCode::FirmwarePackageNotFound;

    auto validationResult = m_firmwareValidator->validate(*resolved.package);
    if (!validationResult.valid) {
        return ErrorCode::FirmwareValidationFailed;
    }

    m_loadedPackage = std::move(resolved.package);
    m_loadedPackageRaw = m_loadedPackage.get();

    return std::move(m_loadedPackage);
}

firmware::FirmwarePackage* FirmwareService::loadedPackage() const noexcept {
    return m_loadedPackage ? m_loadedPackage.get() : m_loadedPackageRaw;
}

Result<void> FirmwareService::flash(const std::string& packagePath) {
    MBOOT_TRY_ASSIGN(pkg, loadFirmwarePackage(packagePath));
    return flash(*pkg);
}

Result<void> FirmwareService::flash(firmware::FirmwarePackage& package) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    auto workflow = m_workflowFactory.createFlashWorkflow(session, &package);
    return m_workflowExec(std::move(workflow));
}

Result<ByteBuffer> FirmwareService::read(uint64_t address, size_t size) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->readMemory(address, size);
}

Result<void> FirmwareService::write(uint64_t address, const ByteBuffer& data) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->writeMemory(address, data);
}

Result<void> FirmwareService::erase(uint64_t address, size_t size) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->eraseMemory(address, size);
}

Result<void> FirmwareService::verify(uint64_t address, const ByteBuffer& expected) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;

    MBOOT_TRY_ASSIGN(readData, session->readMemory(address, expected.size()));

    if (readData != expected) {
        return ErrorCode::FirmwareHashMismatch;
    }
    return {};
}

Result<ByteBuffer> FirmwareService::readPartition(const std::string& name) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->readPartition(name);
}

Result<void> FirmwareService::writePartition(const std::string& name, const ByteBuffer& data) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->writePartition(name, data);
}

Result<void> FirmwareService::erasePartition(const std::string& name) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;
    return session->erasePartition(name);
}

Result<ByteBuffer> FirmwareService::backup(const std::string& partition) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;

    auto workflow = m_workflowFactory.createBackupWorkflow(session, partition);
    MBOOT_TRY(m_workflowExec(std::move(workflow)));

    return session->readPartition(partition);
}

Result<void> FirmwareService::restore(const std::string& partition, const ByteBuffer& data) {
    auto* session = activeSession();
    if (!session) return ErrorCode::SessionNotConnected;

    MBOOT_TRY(session->writePartition(partition, data));

    auto workflow = m_workflowFactory.createRestoreWorkflow(session, partition);
    return m_workflowExec(std::move(workflow));
}

} // namespace runtime
} // namespace mbootcore
