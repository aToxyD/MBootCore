#include "WorkflowService.hpp"

#include <mbootcore/session/DeviceSession.hpp>

#include <sstream>

namespace mbootcore {
namespace runtime {

WorkflowService::WorkflowService(
    std::unique_ptr<workflow::WorkflowFactory> workflowFactory,
    std::unique_ptr<job::JobContext> jobContext,
    std::unique_ptr<job::JobScheduler> jobScheduler,
    std::unique_ptr<pipeline::BootPipeline> bootPipeline,
    ILogger& logger,
    IDeviceService& deviceService,
    IFirmwareService& firmwareService)
    : m_workflowFactory(std::move(workflowFactory))
    , m_jobContext(std::move(jobContext))
    , m_jobScheduler(std::move(jobScheduler))
    , m_bootPipeline(std::move(bootPipeline))
    , m_logger(logger)
    , m_deviceService(deviceService)
    , m_firmwareService(firmwareService)
{
}

Result<void>
WorkflowService::executeWorkflow(std::unique_ptr<workflow::IWorkflow> workflow) {
    m_currentWorkflow = std::move(workflow);

    MBOOT_TRY(m_currentWorkflow->prepare());

    auto runResult = m_currentWorkflow->run();

    m_currentWorkflow.reset();
    return runResult;
}

Result<void>
WorkflowService::executeWorkflow(const std::string& workflowType) {
    if (!m_deviceService.activeSession())
        return ErrorCode::SessionNotConnected;

    std::unique_ptr<workflow::IWorkflow> wf;

    if (workflowType == "flash") {
        wf = m_workflowFactory->createFlashWorkflow(
            m_deviceService.activeSession(),
            m_firmwareService.loadedPackage());
    } else if (workflowType == "backup") {
        wf = m_workflowFactory->createBackupWorkflow(m_deviceService.activeSession());
    } else if (workflowType == "restore") {
        wf = m_workflowFactory->createRestoreWorkflow(m_deviceService.activeSession());
    } else if (workflowType == "verify") {
        wf = m_workflowFactory->createVerificationWorkflow(m_deviceService.activeSession());
    } else if (workflowType == "recovery") {
        wf = m_workflowFactory->createRecoveryWorkflow(m_deviceService.activeSession());
    } else {
        return ErrorCode::InvalidArgument;
    }

    return executeWorkflow(std::move(wf));
}

Result<void>
WorkflowService::runJob(std::unique_ptr<job::IJob> job) {
    if (!m_jobContext || !m_jobScheduler)
        return ErrorCode::InvalidState;

    m_jobScheduler->enqueue(std::move(job));
    m_jobScheduler->start();

    return {};
}

Result<void>
WorkflowService::runJobs(std::vector<std::unique_ptr<job::IJob>> jobs) {
    if (!m_jobContext || !m_jobScheduler)
        return ErrorCode::InvalidState;

    for (auto& j : jobs) {
        m_jobScheduler->enqueue(std::move(j));
    }
    m_jobScheduler->start();

    return {};
}

void WorkflowService::cancel() {
    if (m_currentWorkflow) m_currentWorkflow->cancel();
    if (m_jobScheduler) m_jobScheduler->stop();
    if (m_bootPipeline) m_bootPipeline->cancel();
}

void WorkflowService::pause() {
    if (m_currentWorkflow) (void)m_currentWorkflow->pause();
    if (m_jobScheduler) m_jobScheduler->pause();
}

void WorkflowService::resume() {
    if (m_currentWorkflow) (void)m_currentWorkflow->resume();
    if (m_jobScheduler) m_jobScheduler->resume();
}

void WorkflowService::reset() {
    if (m_bootPipeline) {
        auto r = m_bootPipeline->reset();
        (void)r;
    }
    if (m_currentWorkflow) {
        (void)m_currentWorkflow->reset();
        m_currentWorkflow.reset();
    }
    if (m_jobScheduler) m_jobScheduler->stop();
}

workflow::WorkflowFactory& WorkflowService::workflowFactory() noexcept {
    return *m_workflowFactory;
}

job::JobScheduler& WorkflowService::jobScheduler() noexcept {
    return *m_jobScheduler;
}

pipeline::BootPipeline& WorkflowService::bootPipeline() noexcept {
    return *m_bootPipeline;
}

} // namespace runtime
} // namespace mbootcore
