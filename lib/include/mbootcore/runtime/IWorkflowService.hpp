#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/job/IJob.hpp>
#include <mbootcore/job/JobTypes.hpp>

#include <memory>
#include <string>
#include <vector>

namespace mbootcore {
namespace runtime {

class IWorkflowService {
public:
    virtual ~IWorkflowService() = default;

    virtual Result<void> executeWorkflow(
        std::unique_ptr<workflow::IWorkflow> workflow) = 0;

    virtual Result<void> executeWorkflow(const std::string& workflowType) = 0;

    virtual Result<void> runJob(std::unique_ptr<job::IJob> job) = 0;

    virtual Result<void> runJobs(
        std::vector<std::unique_ptr<job::IJob>> jobs) = 0;
};

} // namespace runtime
} // namespace mbootcore
