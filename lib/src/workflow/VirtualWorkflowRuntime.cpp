#include <mbootcore/workflow/VirtualWorkflowRuntime.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/workflow/WorkflowExecutor.hpp>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>

namespace mbootcore {
namespace workflow {

VirtualStep::VirtualStep(const std::string& name, WorkflowStepType type,
                         ErrorCode error)
    : m_name(name)
    , m_type(type)
    , m_executeError(error) {}

Result<void> VirtualStep::prepare(WorkflowContext&) {
    m_prepared = true;
    return {};
}

Result<void> VirtualStep::execute(WorkflowContext& context) {
    m_executed = true;
    m_progress = 100.0;

    if (context.isCancelled()) {
        return ErrorCode::Cancelled;
    }

    if (m_executeError != ErrorCode::Success) {
        return m_executeError;
    }

    return {};
}

Result<void> VirtualStep::rollback(WorkflowContext&) {
    m_rolledBack = true;
    if (m_rollbackError != ErrorCode::Success) {
        return m_rollbackError;
    }
    return {};
}

VirtualWorkflowRuntime::VirtualWorkflowRuntime() {
    m_config.stepDelay = std::chrono::milliseconds(10);
}

std::unique_ptr<firmware::FirmwarePackage> VirtualWorkflowRuntime::createFirmwarePackage() {
    if (m_config.simulateCorruptFirmware) {
        return m_packageGen.generateBadHash();
    }
    return m_packageGen.generateGoodPackage();
}

std::unique_ptr<WorkflowEngine> VirtualWorkflowRuntime::createWorkflow() {
    auto engine = std::make_unique<WorkflowEngine>();
    WorkflowOptions opts;
    opts.autoRollback = true;
    opts.retryCount = 3;
    opts.timeout = std::chrono::milliseconds(5000);
    engine->setOptions(opts);
    return engine;
}

WorkflowContext VirtualWorkflowRuntime::createContext() {
    WorkflowContext ctx;
    return ctx;
}

VirtualStep* VirtualWorkflowRuntime::createStep(
    const std::string& name, WorkflowStepType type) {

    auto error = ErrorCode::Success;

    if (m_config.simulateTimeout) {
        error = ErrorCode::WorkflowExecutionFailed;
    }

    if (m_config.simulateLoaderFailure &&
        type == WorkflowStepType::UploadLoader) {
        error = ErrorCode::LoaderNotFound;
    }

    if (m_config.simulateVerificationFailure &&
        type == WorkflowStepType::Verify) {
        error = ErrorCode::FirmwareValidationFailed;
    }

    auto step = std::make_unique<VirtualStep>(name, type, error);
    auto* raw = step.get();
    m_ownedSteps.push_back(std::move(step));
    return raw;
}

VirtualStep* VirtualWorkflowRuntime::addStepToWorkflow(WorkflowEngine& engine,
                                                        const std::string& name,
                                                        WorkflowStepType type) {
    auto error = ErrorCode::Success;

    if (m_config.simulateTimeout) {
        error = ErrorCode::WorkflowExecutionFailed;
    }

    if (m_config.simulateLoaderFailure &&
        type == WorkflowStepType::UploadLoader) {
        error = ErrorCode::LoaderNotFound;
    }

    if (m_config.simulateVerificationFailure &&
        type == WorkflowStepType::Verify) {
        error = ErrorCode::FirmwareValidationFailed;
    }

    auto step = std::make_unique<VirtualStep>(name, type, error);
    auto* raw = step.get();
    m_steps[name] = raw;
    engine.addStep(std::move(step));
    return raw;
}

size_t VirtualWorkflowRuntime::buildFlashWorkflow(WorkflowEngine& engine) {
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    addStepToWorkflow(engine, "connect", WST::Connect);
    addStepToWorkflow(engine, "detect", WST::Detect);
    addStepToWorkflow(engine, "negotiate", WST::Negotiate);
    addStepToWorkflow(engine, "upload_loader", WST::UploadLoader);
    addStepToWorkflow(engine, "flash", WST::Flash);
    addStepToWorkflow(engine, "verify", WST::Verify);
    addStepToWorkflow(engine, "reboot", WST::Reboot);

    return m_steps.size();
}

size_t VirtualWorkflowRuntime::buildBackupWorkflow(WorkflowEngine& engine,
                                                     const std::string& partition) {
    (void)partition;
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    addStepToWorkflow(engine, "connect", WST::Connect);
    addStepToWorkflow(engine, "detect", WST::Detect);
    addStepToWorkflow(engine, "backup", WST::Backup);

    return m_steps.size();
}

size_t VirtualWorkflowRuntime::buildRestoreWorkflow(WorkflowEngine& engine,
                                                      const std::string& partition) {
    (void)partition;
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    addStepToWorkflow(engine, "connect", WST::Connect);
    addStepToWorkflow(engine, "detect", WST::Detect);
    addStepToWorkflow(engine, "restore", WST::Restore);

    return m_steps.size();
}

size_t VirtualWorkflowRuntime::buildRecoveryWorkflow(WorkflowEngine& engine) {
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    addStepToWorkflow(engine, "connect", WST::Connect);
    addStepToWorkflow(engine, "detect", WST::Detect);

    auto failStep = std::make_unique<VirtualStep>(
        "upload_loader", WST::UploadLoader,
        ErrorCode::LoaderNotFound);
    auto* raw = failStep.get();
    m_steps["upload_loader"] = raw;
    engine.addStep(std::move(failStep));

    emitEvent("Recovery workflow built with failing upload_loader step");
    return m_steps.size();
}

size_t VirtualWorkflowRuntime::buildCancelWorkflow(WorkflowEngine& engine) {
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    addStepToWorkflow(engine, "connect", WST::Connect);
    addStepToWorkflow(engine, "detect", WST::Detect);

    auto cancelStep = std::make_unique<VirtualStep>(
        "negotiate", WST::Negotiate,
        ErrorCode::Cancelled);
    auto* raw = cancelStep.get();
    m_steps["negotiate"] = raw;
    engine.addStep(std::move(cancelStep));

    return m_steps.size();
}

size_t VirtualWorkflowRuntime::buildRollbackWorkflow(WorkflowEngine& engine) {
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    addStepToWorkflow(engine, "connect", WST::Connect);
    addStepToWorkflow(engine, "detect", WST::Detect);

    auto failStep = std::make_unique<VirtualStep>(
        "flash", WST::Flash,
        ErrorCode::JobFailed);
    auto* raw = failStep.get();
    m_steps["flash"] = raw;
    engine.addStep(std::move(failStep));

    return m_steps.size();
}

size_t VirtualWorkflowRuntime::buildLargeWorkflow(WorkflowEngine& engine,
                                                    size_t repeatCount) {
    m_steps.clear();
    m_ownedSteps.clear();

    using WST = WorkflowStepType;
    for (size_t i = 0; i < repeatCount; ++i) {
        addStepToWorkflow(engine,
                          "connect_" + std::to_string(i), WST::Connect);
        addStepToWorkflow(engine,
                          "detect_" + std::to_string(i), WST::Detect);
        addStepToWorkflow(engine,
                          "flash_" + std::to_string(i), WST::Flash);
    }

    return m_steps.size();
}

VirtualStep* VirtualWorkflowRuntime::getStep(
    const std::string& name) const {
    auto it = m_steps.find(name);
    return it != m_steps.end() ? it->second : nullptr;
}

void VirtualWorkflowRuntime::reset() {
    m_steps.clear();
    m_ownedSteps.clear();
    m_packageGen.reset();
    m_config = VirtualWorkflowConfig{};
}

void VirtualWorkflowRuntime::emitEvent(const std::string& event) {
    if (m_callback) {
        m_callback(event);
    }
}

std::string VirtualWorkflowRuntime::makeStepName(WorkflowStepType type) {
    switch (type) {
        case WorkflowStepType::Connect:      return "connect";
        case WorkflowStepType::Detect:       return "detect";
        case WorkflowStepType::Negotiate:    return "negotiate";
        case WorkflowStepType::UploadLoader: return "upload_loader";
        case WorkflowStepType::Flash:        return "flash";
        case WorkflowStepType::Verify:       return "verify";
        case WorkflowStepType::GPT:          return "gpt";
        case WorkflowStepType::Backup:       return "backup";
        case WorkflowStepType::Restore:      return "restore";
        case WorkflowStepType::Reboot:       return "reboot";
        case WorkflowStepType::Disconnect:   return "disconnect";
        default: return "custom";
    }
}

} // namespace workflow
} // namespace mbootcore
