#pragma once

#include <mbootcore/runtime/IWorkflowService.hpp>
#include <mbootcore/runtime/IDeviceService.hpp>
#include <mbootcore/runtime/IFirmwareService.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/workflow/WorkflowFactory.hpp>
#include <mbootcore/job/JobScheduler.hpp>
#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>

#include <memory>

namespace mbootcore {
namespace runtime {

class WorkflowService final : public IWorkflowService {
public:
    WorkflowService(
        std::unique_ptr<workflow::WorkflowFactory> workflowFactory,
        std::unique_ptr<job::JobContext> jobContext,
        std::unique_ptr<job::JobScheduler> jobScheduler,
        std::unique_ptr<pipeline::BootPipeline> bootPipeline,
        ILogger& logger,
        IDeviceService& deviceService,
        IFirmwareService& firmwareService);

    ~WorkflowService() override = default;

    // IWorkflowService
    Result<void> executeWorkflow(std::unique_ptr<workflow::IWorkflow> workflow) override;
    Result<void> executeWorkflow(const std::string& workflowType) override;
    Result<void> runJob(std::unique_ptr<job::IJob> job) override;
    Result<void> runJobs(std::vector<std::unique_ptr<job::IJob>> jobs) override;

    // Lifecycle helpers (called by Runtime)
    void cancel();
    void pause();
    void resume();
    void reset();

    // Accessors
    workflow::WorkflowFactory& workflowFactory() noexcept;
    job::JobScheduler& jobScheduler() noexcept;
    pipeline::BootPipeline& bootPipeline() noexcept;

private:
    std::unique_ptr<workflow::WorkflowFactory> m_workflowFactory;
    std::unique_ptr<job::JobContext> m_jobContext;
    std::unique_ptr<job::JobScheduler> m_jobScheduler;
    std::unique_ptr<pipeline::BootPipeline> m_bootPipeline;
    std::unique_ptr<workflow::IWorkflow> m_currentWorkflow;

    ILogger& m_logger;
    IDeviceService& m_deviceService;
    IFirmwareService& m_firmwareService;
};

} // namespace runtime
} // namespace mbootcore
