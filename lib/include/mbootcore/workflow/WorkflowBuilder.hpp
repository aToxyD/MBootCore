#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <memory>
#include <vector>
#include <string>

namespace mbootcore {
namespace workflow {

class WorkflowBuilder {
public:
    WorkflowBuilder() = default;

    WorkflowBuilder& connect(int retries = 3);
    WorkflowBuilder& detect();
    WorkflowBuilder& negotiate();
    WorkflowBuilder& uploadLoader(const std::string& loaderPath = {});
    WorkflowBuilder& flash();
    WorkflowBuilder& verify();
    WorkflowBuilder& gpt();
    WorkflowBuilder& backup(const std::string& partition = {});
    WorkflowBuilder& restore(const std::string& partition = {});
    WorkflowBuilder& reboot();
    WorkflowBuilder& disconnect();
    WorkflowBuilder& custom(std::unique_ptr<IWorkflowStep> step);

    WorkflowBuilder& setOptions(const WorkflowOptions& options);
    WorkflowBuilder& setContext(WorkflowContext context);

    std::unique_ptr<IWorkflow> build();

    size_t stepCount() const noexcept { return m_steps.size(); }
    void clear();

private:
    struct StepEntry {
        WorkflowStepType type;
        std::unique_ptr<IWorkflowStep> customStep;
        std::string partition;
        std::string loaderPath;
    };

    std::vector<StepEntry> m_steps;
    WorkflowOptions m_options;
    WorkflowContext m_context;
};

} // namespace workflow
} // namespace mbootcore
