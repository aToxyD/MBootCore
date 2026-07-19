#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/job/JobPipeline.hpp>
#include <mbootcore/session/DeviceSession.hpp>

#include <memory>

namespace mbootcore {
namespace workflow {

class WorkflowExecutor {
public:
    WorkflowExecutor() = default;

    Result<void> executeConnect(WorkflowContext& context);
    Result<void> executeDetect(WorkflowContext& context);
    Result<void> executeNegotiate(WorkflowContext& context);
    Result<void> executeUploadLoader(WorkflowContext& context);
    Result<void> executeFlash(WorkflowContext& context);
    Result<void> executeVerify(WorkflowContext& context);
    Result<void> executeGPT(WorkflowContext& context);
    Result<void> executeBackup(WorkflowContext& context, const std::string& partition = {});
    Result<void> executeRestore(WorkflowContext& context, const std::string& partition = {});
    Result<void> executeReboot(WorkflowContext& context);
    Result<void> executeDisconnect(WorkflowContext& context);

    size_t executedSteps() const noexcept { return m_executedSteps; }
    size_t failedSteps() const noexcept { return m_failedSteps; }

private:
    size_t m_executedSteps{0};
    size_t m_failedSteps{0};
};

} // namespace workflow
} // namespace mbootcore
