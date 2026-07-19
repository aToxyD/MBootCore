#pragma once

#include <mbootcore/domain/Error.hpp>
#include <cstdint>
#include <string>

namespace mbootcore {
namespace workflow {

enum class WorkflowStepType : uint32_t;

} // namespace workflow
} // namespace mbootcore

namespace mbootcore {
namespace workflow {

struct WorkflowContext;

class IWorkflowStep {
public:
    virtual ~IWorkflowStep() = default;

    virtual Result<void> prepare(WorkflowContext& context) = 0;
    virtual Result<void> execute(WorkflowContext& context) = 0;
    virtual Result<void> rollback(WorkflowContext& context) = 0;

    virtual std::string name() const noexcept = 0;
    virtual WorkflowStepType type() const noexcept = 0;
    virtual double progress() const noexcept = 0;
};

} // namespace workflow
} // namespace mbootcore
