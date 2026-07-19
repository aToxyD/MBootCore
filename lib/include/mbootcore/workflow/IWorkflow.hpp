#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <memory>
#include <vector>

namespace mbootcore {
namespace workflow {

class IWorkflow {
public:
    virtual ~IWorkflow() = default;

    virtual Result<void> prepare() = 0;
    virtual Result<void> run() = 0;
    virtual Result<void> pause() = 0;
    virtual Result<void> resume() = 0;
    virtual void cancel() noexcept = 0;
    virtual Result<void> rollback() = 0;
    virtual Result<void> reset() = 0;

    virtual WorkflowProgress progress() const noexcept = 0;
    virtual WorkflowResult result() const noexcept = 0;
    virtual WorkflowStatistics statistics() const noexcept = 0;
    virtual std::vector<std::string> history() const = 0;

    virtual WorkflowState state() const noexcept = 0;
    virtual std::string stateString() const = 0;

    virtual void setContext(WorkflowContext context) = 0;
    virtual WorkflowContext& context() noexcept = 0;
    virtual const WorkflowContext& context() const noexcept = 0;

    virtual void setOptions(const WorkflowOptions& options) = 0;
    virtual const WorkflowOptions& options() const noexcept = 0;
};

} // namespace workflow
} // namespace mbootcore
