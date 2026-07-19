#include <mbootcore/workflow/WorkflowBuilder.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <utility>

namespace mbootcore {
namespace workflow {

namespace {

class NamedStep : public IWorkflowStep {
public:
    NamedStep(std::string name, WorkflowStepType type)
        : m_name(std::move(name)), m_type(type) {}

    Result<void> prepare(WorkflowContext&) override { return {}; }
    Result<void> execute(WorkflowContext&) override { return {}; }
    Result<void> rollback(WorkflowContext&) override { return {}; }

    std::string name() const noexcept override { return m_name; }
    WorkflowStepType type() const noexcept override { return m_type; }
    double progress() const noexcept override { return 0.0; }

private:
    std::string m_name;
    WorkflowStepType m_type;
};

} // anonymous namespace

WorkflowBuilder& WorkflowBuilder::connect(int retries) {
    (void)retries;
    m_steps.push_back({WorkflowStepType::Connect, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::detect() {
    m_steps.push_back({WorkflowStepType::Detect, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::negotiate() {
    m_steps.push_back({WorkflowStepType::Negotiate, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::uploadLoader(const std::string& loaderPath) {
    StepEntry e;
    e.type = WorkflowStepType::UploadLoader;
    e.loaderPath = loaderPath;
    m_steps.push_back(std::move(e));
    return *this;
}

WorkflowBuilder& WorkflowBuilder::flash() {
    m_steps.push_back({WorkflowStepType::Flash, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::verify() {
    m_steps.push_back({WorkflowStepType::Verify, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::gpt() {
    m_steps.push_back({WorkflowStepType::GPT, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::backup(const std::string& partition) {
    StepEntry e;
    e.type = WorkflowStepType::Backup;
    e.partition = partition;
    m_steps.push_back(std::move(e));
    return *this;
}

WorkflowBuilder& WorkflowBuilder::restore(const std::string& partition) {
    StepEntry e;
    e.type = WorkflowStepType::Restore;
    e.partition = partition;
    m_steps.push_back(std::move(e));
    return *this;
}

WorkflowBuilder& WorkflowBuilder::reboot() {
    m_steps.push_back({WorkflowStepType::Reboot, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::disconnect() {
    m_steps.push_back({WorkflowStepType::Disconnect, nullptr, {}, {}});
    return *this;
}

WorkflowBuilder& WorkflowBuilder::custom(std::unique_ptr<IWorkflowStep> step) {
    StepEntry e;
    e.type = WorkflowStepType::Custom;
    e.customStep = std::move(step);
    m_steps.push_back(std::move(e));
    return *this;
}

WorkflowBuilder& WorkflowBuilder::setOptions(const WorkflowOptions& options) {
    m_options = options;
    return *this;
}

WorkflowBuilder& WorkflowBuilder::setContext(WorkflowContext context) {
    m_context = std::move(context);
    return *this;
}

std::unique_ptr<IWorkflow> WorkflowBuilder::build() {
    auto engine = std::make_unique<WorkflowEngine>();

    for (auto& entry : m_steps) {
        if (entry.type == WorkflowStepType::Custom && entry.customStep) {
            engine->addStep(std::move(entry.customStep));
        } else {
            auto stepName = [&]() -> std::string {
                switch (entry.type) {
                    case WorkflowStepType::Connect: return "connect";
                    case WorkflowStepType::Detect: return "detect";
                    case WorkflowStepType::Negotiate: return "negotiate";
                    case WorkflowStepType::UploadLoader: return "uploadLoader";
                    case WorkflowStepType::Flash: return "flash";
                    case WorkflowStepType::Verify: return "verify";
                    case WorkflowStepType::GPT: return "gpt";
                    case WorkflowStepType::Backup: return "backup";
                    case WorkflowStepType::Restore: return "restore";
                    case WorkflowStepType::Reboot: return "reboot";
                    case WorkflowStepType::Disconnect: return "disconnect";
                    default: return "step";
                }
            }();
            engine->addStep(std::make_unique<NamedStep>(stepName, entry.type));
        }
    }

    engine->setOptions(m_options);
    if (!m_context.properties.empty() || m_context.session != nullptr) {
        engine->setContext(std::move(m_context));
    }

    return engine;
}

void WorkflowBuilder::clear() {
    m_steps.clear();
    m_options = WorkflowOptions{};
}

} // namespace workflow
} // namespace mbootcore
