#include <catch2/catch_test_macros.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/WorkflowBuilder.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/workflow/WorkflowExecutor.hpp>
#include <mbootcore/workflow/WorkflowRecovery.hpp>
#include <mbootcore/workflow/WorkflowProgressEngine.hpp>
#include <mbootcore/workflow/WorkflowHistory.hpp>
#include <mbootcore/workflow/WorkflowFactory.hpp>
#include <mbootcore/workflow/VirtualWorkflowRuntime.hpp>

#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace mbootcore::workflow;
using namespace mbootcore;

// ─── Helpers ─────────────────────────────────────────────────────────────────────

static WorkflowContext makeTestContext() {
    WorkflowContext ctx;
    return ctx;
}

static std::unique_ptr<WorkflowEngine> createEngineWithSteps(size_t count = 3) {
    auto engine = std::make_unique<WorkflowEngine>();
    for (size_t i = 0; i < count; ++i) {
        auto step = std::make_unique<VirtualStep>(
            "step_" + std::to_string(i),
            static_cast<WorkflowStepType>(i));
        engine->addStep(std::move(step));
    }
    return engine;
}

// ─── Test Class ──────────────────────────────────────────────────────────────────

TEST_CASE("WorkflowEngineTest", "[workflow]") {

    //1 — WorkflowTypes tests
    SECTION("test_WorkflowTypes_enumValues") {
    }
    SECTION("test_WorkflowTypes_stateDefault") {
    }
    SECTION("test_WorkflowTypes_statisticsDefault") {
    }
    SECTION("test_WorkflowTypes_progressDefault") {
    }
    SECTION("test_WorkflowTypes_resultDefault") {
    }
    SECTION("test_WorkflowTypes_optionsDefault") {
    }
    SECTION("test_WorkflowTypes_contextDefault") {

    //2 — IWorkflow tests via WorkflowEngine
    }
    SECTION("test_IWorkflow_prepare") {
    }
    SECTION("test_IWorkflow_prepareTwiceFails") {
    }
    SECTION("test_IWorkflow_runEmptyWorkflow") {
    }
    SECTION("test_IWorkflow_runSimpleWorkflow") {
    }
    SECTION("test_IWorkflow_cancel") {
    }
    SECTION("test_IWorkflow_reset") {

    //3 — WorkflowEngine tests
    }
    SECTION("test_WorkflowEngine_stateTransitions") {
    }
    SECTION("test_WorkflowEngine_addRemoveSteps") {
    }
    SECTION("test_WorkflowEngine_insertStep") {
    }
    SECTION("test_WorkflowEngine_clearSteps") {
    }
    SECTION("test_WorkflowEngine_findStep") {

    //4 — WorkflowBuilder tests
    }
    SECTION("test_WorkflowBuilder_connectBuildsStep") {
    }
    SECTION("test_WorkflowBuilder_detectBuildsStep") {
    }
    SECTION("test_WorkflowBuilder_flashBuildsStep") {
    }
    SECTION("test_WorkflowBuilder_fullBuild") {
    }
    SECTION("test_WorkflowBuilder_clear") {

    //5 — IWorkflowStep tests via VirtualStep
    }
    SECTION("test_IWorkflowStep_prepareExecuteRollback") {
    }
    SECTION("test_IWorkflowStep_executeCancelled") {
    }
    SECTION("test_IWorkflowStep_executeFails") {
    }
    SECTION("test_IWorkflowStep_rollbackFails") {

    //6 — Built-in workflow step execution
    }
    SECTION("test_WorkflowExecutor_connect") {
    }
    SECTION("test_WorkflowExecutor_disconnect") {
    }
    SECTION("test_WorkflowExecutor_verifyNoSession") {
    }
    SECTION("test_WorkflowExecutor_flashNoPackage") {

    // — WorkflowExecutor tests
    }
    SECTION("test_WorkflowExecutor_executedStepsCount") {
    }
    SECTION("test_WorkflowExecutor_failedStepsCount") {
    }
    SECTION("test_WorkflowExecutor_uploadLoaderNoSession") {

    //8 — WorkflowRecovery tests
    }
    SECTION("test_WorkflowRecovery_defaultStrategy") {
    }
    SECTION("test_WorkflowRecovery_customStrategy") {
    }
    SECTION("test_WorkflowRecovery_perStepStrategy") {
    }
    SECTION("test_WorkflowRecovery_clearCustom") {
    }
    SECTION("test_WorkflowRecovery_cancelledAction") {
    }
    SECTION("test_WorkflowRecovery_verifyAction") {

    //9 — WorkflowProgressEngine tests
    }
    SECTION("test_WorkflowProgressEngine_defaults") {
    }
    SECTION("test_WorkflowProgressEngine_setTotalSteps") {
    }
    SECTION("test_WorkflowProgressEngine_stepProgress") {
    }
    SECTION("test_WorkflowProgressEngine_markCompleted") {
    }
    SECTION("test_WorkflowProgressEngine_markFailed") {
    }
    SECTION("test_WorkflowProgressEngine_markCancelled") {
    }
    SECTION("test_WorkflowProgressEngine_eta") {
    }
    SECTION("test_WorkflowProgressEngine_callback") {

    //10 — WorkflowHistory tests
    }
    SECTION("test_WorkflowHistory_addAndCount") {
    }
    SECTION("test_WorkflowHistory_recent") {
    }
    SECTION("test_WorkflowHistory_filterBySuccess") {
    }
    SECTION("test_WorkflowHistory_filterByVendor") {
    }
    SECTION("test_WorkflowHistory_filterByProtocol") {
    }
    SECTION("test_WorkflowHistory_clear") {
    }
    SECTION("test_WorkflowHistory_exportText") {
    }
    SECTION("test_WorkflowHistory_exportJson") {
    }
    SECTION("test_WorkflowHistory_successFailureCount") {

    //11 — WorkflowFactory tests
    }
    SECTION("test_WorkflowFactory_createFlashWorkflow") {
    }
    SECTION("test_WorkflowFactory_createBackupWorkflow") {
    }
    SECTION("test_WorkflowFactory_createRestoreWorkflow") {
    }
    SECTION("test_WorkflowFactory_createVerificationWorkflow") {
    }
    SECTION("test_WorkflowFactory_createRecoveryWorkflow") {
    }
    SECTION("test_WorkflowFactory_createCustomWorkflow") {

    //12 — VirtualWorkflowRuntime tests
    }
    SECTION("test_VirtualRuntime_createStep") {
    }
    SECTION("test_VirtualRuntime_buildFlashWorkflow") {
    }
    SECTION("test_VirtualRuntime_buildBackupWorkflow") {
    }
    SECTION("test_VirtualRuntime_buildRestoreWorkflow") {
    }
    SECTION("test_VirtualRuntime_buildRecoveryWorkflow") {
    }
    SECTION("test_VirtualRuntime_buildCancelWorkflow") {
    }
    SECTION("test_VirtualRuntime_buildRollbackWorkflow") {
    }
    SECTION("test_VirtualRuntime_buildLargeWorkflow") {
    }
    SECTION("test_VirtualRuntime_createFirmwarePackage") {
    }
    SECTION("test_VirtualRuntime_getStep") {

    //13 — Stress & Integration
    }
    SECTION("test_Rollback") {
    }
    SECTION("test_Retry") {
    }
    SECTION("test_PauseResume") {
    }
    SECTION("test_CancelDuringExecution") {
    }
    SECTION("test_RecoveryAfterFailure") {
    }
    SECTION("test_WorkflowHistoryRoundTrip") {
    }
    SECTION("test_ProgressCallbacks") {
    }
    SECTION("test_StateCallbacks") {
    }
    SECTION("test_ErrorCallbacks") {
    }
    SECTION("test_LargeWorkflow") {
    }
    SECTION("test_EmptyWorkflow") {
    }
    SECTION("test_StepWithCustomResult") {
    }
    SECTION("test_WorkflowOptionsContinueOnWarning") {
    }
    SECTION("test_WorkflowEngineMultipleRuns") {

    // Failure injection
    }
    SECTION("test_FailureInjection_loaderFailure") {
    }
    SECTION("test_FailureInjection_verificationFailure") {
    }
    SECTION("test_FailureInjection_cancelScenario") {
    }
    SECTION("test_FailureInjection_rollbackScenario") {
    }
}

// ───1 — WorkflowTypes ────────────────────────────────────────────────

void test_WorkflowTypes_enumValues() {
    REQUIRE(static_cast<int>(WorkflowState::Created) == 0);
    REQUIRE(static_cast<int>(WorkflowState::Ready) == 1);
    REQUIRE(static_cast<int>(WorkflowState::Running) == 2);
    REQUIRE(static_cast<int>(WorkflowState::Paused) == 3);
    REQUIRE(static_cast<int>(WorkflowState::Cancelled) == 4);
    REQUIRE(static_cast<int>(WorkflowState::Completed) == 5);
    REQUIRE(static_cast<int>(WorkflowState::Failed) == 6);
    REQUIRE(static_cast<int>(WorkflowState::RollingBack) == 7);
    REQUIRE(static_cast<int>(WorkflowStepType::Connect) == 0);
    REQUIRE(static_cast<int>(WorkflowStepType::Detect) == 1);
    REQUIRE(static_cast<int>(WorkflowStepType::Custom) == 99);
    REQUIRE(static_cast<int>(WorkflowPriority::Normal) == 1);
}

void test_WorkflowTypes_stateDefault() {
    WorkflowState s{};
    REQUIRE(s == WorkflowState::Created);
}

void test_WorkflowTypes_statisticsDefault() {
    WorkflowStatistics s;
    REQUIRE(s.elapsed.count() == 0);
    REQUIRE(s.retries == 0u);
    REQUIRE(s.rollbacks == 0u);
    REQUIRE(s.warnings == 0u);
    REQUIRE(s.errors == 0u);
}

void test_WorkflowTypes_progressDefault() {
    WorkflowProgress p;
    REQUIRE(p.overallProgress == 0.0);
    REQUIRE(p.currentStepProgress == 0.0);
    REQUIRE(p.eta.count() == 0);
    REQUIRE(p.bytesTransferred == 0ull);
    REQUIRE(p.currentStep.empty());
}

void test_WorkflowTypes_resultDefault() {
    WorkflowResult r;
    REQUIRE(r.success == false);
    REQUIRE(r.error == ErrorCode::Success);
    REQUIRE(r.message.empty());
}

void test_WorkflowTypes_optionsDefault() {
    WorkflowOptions opts;
    REQUIRE(opts.autoRollback == true);
    REQUIRE(opts.retryCount == 3);
    REQUIRE(opts.continueOnWarning == false);
}

void test_WorkflowTypes_contextDefault() {
    WorkflowContext ctx;
    REQUIRE(ctx.session == nullptr);
    REQUIRE(ctx.bootPipeline == nullptr);
    REQUIRE(ctx.jobPipeline == nullptr);
    REQUIRE(ctx.firmwarePackage == nullptr);
    REQUIRE(ctx.flashDevice == nullptr);
    REQUIRE(!ctx.isCancelled());
    ctx.properties["cancelled"] = "true";
    REQUIRE(ctx.isCancelled());
}

// ───2 — IWorkflow ─────────────────────────────────────────────────────

void test_IWorkflow_prepare() {
    auto engine = createEngineWithSteps(2);
    REQUIRE(engine->state() == WorkflowState::Created);
    auto result = engine->prepare();
    REQUIRE(result.isOk());
    REQUIRE(engine->state() == WorkflowState::Ready);
}

void test_IWorkflow_prepareTwiceFails() {
    auto engine = createEngineWithSteps(2);
    engine->prepare();
    auto result = engine->prepare();
    REQUIRE(result.isError());
}

void test_IWorkflow_runEmptyWorkflow() {
    auto engine = std::make_unique<WorkflowEngine>();
    auto result = engine->run();
    REQUIRE(result.isOk());
    REQUIRE(engine->state() == WorkflowState::Completed);
}

void test_IWorkflow_runSimpleWorkflow() {
    auto engine = createEngineWithSteps(3);
    auto result = engine->run();
    REQUIRE(result.isOk());
    REQUIRE(engine->state() == WorkflowState::Completed);
    REQUIRE(engine->result().success);
}

void test_IWorkflow_cancel() {
    auto engine = createEngineWithSteps(3);
    engine->cancel();
    REQUIRE(engine->state() == WorkflowState::Cancelled);
}

void test_IWorkflow_reset() {
    auto engine = createEngineWithSteps(3);
    engine->run();
    engine->reset();
    REQUIRE(engine->state() == WorkflowState::Created);
    REQUIRE(engine->stepCount() == 0u);
}

// ───3 — WorkflowEngine ────────────────────────────────────────────────

void test_WorkflowEngine_stateTransitions() {
    auto engine = createEngineWithSteps(2);
    REQUIRE(engine->state() == WorkflowState::Created);
    engine->prepare();
    REQUIRE(engine->state() == WorkflowState::Ready);
    engine->run();
    REQUIRE(engine->state() == WorkflowState::Completed);
}

void test_WorkflowEngine_addRemoveSteps() {
    WorkflowEngine engine;
    REQUIRE(engine.stepCount() == 0u);
    engine.addStep(std::make_unique<VirtualStep>("a", WorkflowStepType::Connect));
    REQUIRE(engine.stepCount() == 1u);
    engine.addStep(std::make_unique<VirtualStep>("b", WorkflowStepType::Detect));
    REQUIRE(engine.stepCount() == 2u);
    engine.removeStep("a");
    REQUIRE(engine.stepCount() == 1u);
}

void test_WorkflowEngine_insertStep() {
    WorkflowEngine engine;
    engine.addStep(std::make_unique<VirtualStep>("a", WorkflowStepType::Connect));
    engine.addStep(std::make_unique<VirtualStep>("c", WorkflowStepType::Flash));
    engine.insertStep(1, std::make_unique<VirtualStep>("b", WorkflowStepType::Detect));
    REQUIRE(engine.stepCount() == 3u);
    REQUIRE(engine.findStep("b") != nullptr);
}

void test_WorkflowEngine_clearSteps() {
    auto engine = createEngineWithSteps(3);
    engine->clearSteps();
    REQUIRE(engine->stepCount() == 0u);
}

void test_WorkflowEngine_findStep() {
    auto engine = createEngineWithSteps(3);
    auto* step = engine->findStep("step_1");
    REQUIRE(step != nullptr);
    REQUIRE(step->name() == "step_1");
    auto* missing = engine->findStep("nonexistent");
    REQUIRE(missing == nullptr);
}

// ───4 — WorkflowBuilder ───────────────────────────────────────────────

void test_WorkflowBuilder_connectBuildsStep() {
    WorkflowBuilder builder;
    builder.connect();
    auto workflow = builder.build();
    REQUIRE(workflow != nullptr);
    REQUIRE(workflow->state() == WorkflowState::Created);
}

void test_WorkflowBuilder_detectBuildsStep() {
    WorkflowBuilder builder;
    builder.detect();
    auto workflow = builder.build();
    REQUIRE(workflow != nullptr);
}

void test_WorkflowBuilder_flashBuildsStep() {
    WorkflowBuilder builder;
    builder.flash();
    auto workflow = builder.build();
    REQUIRE(workflow != nullptr);
}

void test_WorkflowBuilder_fullBuild() {
    WorkflowBuilder builder;
    builder
        .connect()
        .detect()
        .negotiate()
        .uploadLoader()
        .flash()
        .verify()
        .reboot()
        .disconnect();

    auto workflow = builder.build();
    REQUIRE(workflow != nullptr);
}

void test_WorkflowBuilder_clear() {
    WorkflowBuilder builder;
    builder.connect().detect().flash();
    REQUIRE(builder.stepCount() == 3u);
    builder.clear();
    REQUIRE(builder.stepCount() == 0u);
}

// ───5 — IWorkflowStep ─────────────────────────────────────────────────

void test_IWorkflowStep_prepareExecuteRollback() {
    auto ctx = makeTestContext();
    auto step = std::make_unique<VirtualStep>("test", WorkflowStepType::Custom);
    REQUIRE(step->prepare(ctx).isOk());
    REQUIRE(step->wasPrepared());
    REQUIRE(step->execute(ctx).isOk());
    REQUIRE(step->wasExecuted());
    REQUIRE(step->rollback(ctx).isOk());
    REQUIRE(step->wasRolledBack());
}

void test_IWorkflowStep_executeCancelled() {
    WorkflowContext ctx;
    ctx.properties["cancelled"] = "true";
    auto step = std::make_unique<VirtualStep>("test", WorkflowStepType::Custom);
    auto result = step->execute(ctx);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::Cancelled);
}

void test_IWorkflowStep_executeFails() {
    auto ctx = makeTestContext();
    auto step = std::make_unique<VirtualStep>(
        "fail", WorkflowStepType::Custom,
        ErrorCode::JobFailed);
    auto result = step->execute(ctx);
    REQUIRE(result.isError());
}

void test_IWorkflowStep_rollbackFails() {
    auto ctx = makeTestContext();
    auto step = std::make_unique<VirtualStep>("rb", WorkflowStepType::Custom);
    step->setRollbackError(ErrorCode::WorkflowRollbackFailed);
    auto result = step->rollback(ctx);
    REQUIRE(result.isError());
}

// ─── WorkflowExecutor ────────────────────────────────────────────────

void test_WorkflowExecutor_connect() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    auto result = exec.executeConnect(ctx);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::SessionNotConnected);
    REQUIRE(exec.failedSteps() == 1u);
}

void test_WorkflowExecutor_disconnect() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    auto result = exec.executeDisconnect(ctx);
    REQUIRE(result.isOk());
    REQUIRE(exec.executedSteps() == 1u);
}

void test_WorkflowExecutor_verifyNoSession() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    auto result = exec.executeVerify(ctx);
    REQUIRE(result.isError());
}

void test_WorkflowExecutor_flashNoPackage() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    auto result = exec.executeFlash(ctx);
    REQUIRE(result.isError());
}

void test_WorkflowExecutor_executedStepsCount() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    exec.executeDisconnect(ctx);
    REQUIRE(exec.executedSteps() == 1u);
}

void test_WorkflowExecutor_failedStepsCount() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    exec.executeConnect(ctx);
    REQUIRE(exec.failedSteps() == 1u);
}

void test_WorkflowExecutor_uploadLoaderNoSession() {
    WorkflowExecutor exec;
    WorkflowContext ctx;
    auto result = exec.executeUploadLoader(ctx);
    REQUIRE(result.isError());
}

// ───8 — WorkflowRecovery ──────────────────────────────────────────────

void test_WorkflowRecovery_defaultStrategy() {
    WorkflowRecovery recovery;
    auto decision = recovery.decide(WorkflowStepType::Connect, "connect",
                                     ErrorCode::TransportError);
    REQUIRE(decision.action == RecoveryAction::Retry);
}

void test_WorkflowRecovery_customStrategy() {
    class AlwaysAbortStrategy : public IRecoveryStrategy {
    public:
        RecoveryDecision decide(WorkflowStepType, const std::string&,
                                 ErrorCode, int) override {
            RecoveryDecision d;
            d.action = RecoveryAction::Abort;
            d.reason = "Always abort";
            return d;
        }
        std::string name() const noexcept override { return "AlwaysAbort"; }
    };

    WorkflowRecovery recovery;
    recovery.setStrategy(std::make_unique<AlwaysAbortStrategy>());
    auto decision = recovery.decide(WorkflowStepType::Connect, "connect",
                                     ErrorCode::TransportError);
    REQUIRE(decision.action == RecoveryAction::Abort);
}

void test_WorkflowRecovery_perStepStrategy() {
    class SkipStrategy : public IRecoveryStrategy {
    public:
        RecoveryDecision decide(WorkflowStepType, const std::string&,
                                 ErrorCode, int) override {
            RecoveryDecision d;
            d.action = RecoveryAction::Skip;
            return d;
        }
        std::string name() const noexcept override { return "Skip"; }
    };

    WorkflowRecovery recovery;
    recovery.setCustomRecovery(WorkflowStepType::Verify,
                               std::make_unique<SkipStrategy>());
    auto decision = recovery.decide(WorkflowStepType::Verify, "verify",
                                     ErrorCode::FirmwareValidationFailed);
    REQUIRE(decision.action == RecoveryAction::Skip);
}

void test_WorkflowRecovery_clearCustom() {
    WorkflowRecovery recovery;
    recovery.setCustomRecovery(WorkflowStepType::Verify,
                               std::make_unique<DefaultRecoveryStrategy>());
    REQUIRE(recovery.strategyCount() == 2u);
    recovery.clearCustomRecoveries();
    REQUIRE(recovery.strategyCount() == 1u);
}

void test_WorkflowRecovery_cancelledAction() {
    DefaultRecoveryStrategy strategy;
    auto decision = strategy.decide(WorkflowStepType::Flash, "flash",
                                     ErrorCode::Cancelled, 0);
    REQUIRE(decision.action == RecoveryAction::Abort);
}

void test_WorkflowRecovery_verifyAction() {
    DefaultRecoveryStrategy strategy;
    auto decision = strategy.decide(WorkflowStepType::Verify, "verify",
                                     ErrorCode::FirmwareValidationFailed, 0);
    REQUIRE(decision.action == RecoveryAction::Abort);
}

// ───9 — WorkflowProgressEngine ────────────────────────────────────────

void test_WorkflowProgressEngine_defaults() {
    WorkflowProgressEngine pe;
    auto p = pe.currentProgress();
    REQUIRE(p.overallProgress == 0.0);
    REQUIRE(p.currentStep.empty());
}

void test_WorkflowProgressEngine_setTotalSteps() {
    WorkflowProgressEngine pe;
    pe.setTotalSteps(5);
    REQUIRE(pe.overallPercentage() == 0.0);
}

void test_WorkflowProgressEngine_stepProgress() {
    WorkflowProgressEngine pe;
    pe.setTotalSteps(2);
    pe.setCurrentStep("test", WorkflowStepType::Flash);
    pe.updateStepProgress(50.0);
    auto p = pe.currentProgress();
    REQUIRE(p.currentStep == "test");
    REQUIRE(p.currentStepProgress == 50.0);
}

void test_WorkflowProgressEngine_markCompleted() {
    WorkflowProgressEngine pe;
    pe.setTotalSteps(1);
    pe.setCurrentStep("test", WorkflowStepType::Flash);
    pe.markStepCompleted();
    auto p = pe.currentProgress();
    REQUIRE(p.overallProgress == 100.0);
}

void test_WorkflowProgressEngine_markFailed() {
    WorkflowProgressEngine pe;
    pe.setTotalSteps(1);
    pe.setCurrentStep("test", WorkflowStepType::Flash);
    pe.markStepFailed("error");
    REQUIRE(pe.overallPercentage() == 0.0);
}

void test_WorkflowProgressEngine_markCancelled() {
    WorkflowProgressEngine pe;
    pe.setTotalSteps(2);
    pe.setCurrentStep("step1", WorkflowStepType::Flash);
    pe.markCancelled();
    auto p = pe.currentProgress();
    REQUIRE(p.currentStep == "Cancelled");
}

void test_WorkflowProgressEngine_eta() {
    WorkflowProgressEngine pe;
    pe.setTotalSteps(10);
    pe.setCurrentStep("test", WorkflowStepType::Flash);
    pe.markStepCompleted();
    auto eta = pe.estimatedTimeRemaining();
    REQUIRE(eta.count() == 0);
}

void test_WorkflowProgressEngine_callback() {
    WorkflowProgressEngine pe;
    bool called = false;
    pe.setCallback([&](const WorkflowProgress&) { called = true; });
    pe.setTotalSteps(1);
    pe.setCurrentStep("test", WorkflowStepType::Flash);
    REQUIRE(called);
}

// ───10 — WorkflowHistory ──────────────────────────────────────────────

void test_WorkflowHistory_addAndCount() {
    WorkflowHistory h;
    REQUIRE(h.totalCount() == 0);
    WorkflowHistoryEntry e;
    e.workflowId = "test1";
    e.success = true;
    h.addEntry(e);
    REQUIRE(h.totalCount() == 1u);
}

void test_WorkflowHistory_recent() {
    WorkflowHistory h;
    for (int i = 0; i < 5; ++i) {
        WorkflowHistoryEntry e;
        e.workflowId = "wf_" + std::to_string(i);
        h.addEntry(e);
    }
    auto recent = h.recent(3);
    REQUIRE(recent.size() == 3u);
    REQUIRE(recent[0].workflowId == "wf_2");
}

void test_WorkflowHistory_filterBySuccess() {
    WorkflowHistory h;
    for (int i = 0; i < 4; ++i) {
        WorkflowHistoryEntry e;
        e.workflowId = "wf_" + std::to_string(i);
        e.success = (i % 2 == 0);
        h.addEntry(e);
    }
    auto successes = h.filter(true);
    auto failures = h.filter(false);
    REQUIRE(successes.size() == 2u);
    REQUIRE(failures.size() == 2u);
}

void test_WorkflowHistory_filterByVendor() {
    WorkflowHistory h;
    WorkflowHistoryEntry e1, e2;
    e1.workflowId = "qcom";
    e1.vendor = discovery::Vendor::Qualcomm;
    e2.workflowId = "mtk";
    e2.vendor = discovery::Vendor::MediaTek;
    h.addEntry(e1);
    h.addEntry(e2);
    auto qcom = h.filterByVendor(discovery::Vendor::Qualcomm);
    REQUIRE(qcom.size() == 1u);
    REQUIRE(qcom[0].workflowId == "qcom");
}

void test_WorkflowHistory_filterByProtocol() {
    WorkflowHistory h;
    WorkflowHistoryEntry e;
    e.workflowId = "sahara";
    e.protocol = discovery::ProtocolType::Sahara;
    h.addEntry(e);
    auto filtered = h.filterByProtocol(discovery::ProtocolType::Sahara);
    REQUIRE(filtered.size() == 1u);
}

void test_WorkflowHistory_clear() {
    WorkflowHistory h;
    WorkflowHistoryEntry e;
    h.addEntry(e);
    h.clear();
    REQUIRE(h.totalCount() == 0);
    REQUIRE(h.empty());
}

void test_WorkflowHistory_exportText() {
    WorkflowHistory h;
    WorkflowHistoryEntry e;
    e.workflowId = "test1";
    e.success = true;
    e.duration = std::chrono::milliseconds(100);
    h.addEntry(e);
    auto text = h.exportText();
    REQUIRE(!text.empty());
    REQUIRE(text.find("test1") != std::string::npos);
}

void test_WorkflowHistory_exportJson() {
    WorkflowHistory h;
    WorkflowHistoryEntry e;
    e.workflowId = "test1";
    e.success = true;
    h.addEntry(e);
    auto json = h.exportJson();
    REQUIRE(!json.empty());
    REQUIRE(json.find("\"id\":\"test1\"") != std::string::npos);
}

void test_WorkflowHistory_successFailureCount() {
    WorkflowHistory h;
    for (int i = 0; i < 5; ++i) {
        WorkflowHistoryEntry e;
        e.success = (i < 3);
        h.addEntry(e);
    }
    REQUIRE(h.successCount() == 3u);
    REQUIRE(h.failureCount() == 2u);
}

// ───11 — WorkflowFactory ──────────────────────────────────────────────

void test_WorkflowFactory_createFlashWorkflow() {
    WorkflowFactory factory;
    auto wf = factory.createFlashWorkflow(nullptr, nullptr);
    REQUIRE(wf != nullptr);
    REQUIRE(wf->state() == WorkflowState::Created);
}

void test_WorkflowFactory_createBackupWorkflow() {
    WorkflowFactory factory;
    auto wf = factory.createBackupWorkflow(nullptr);
    REQUIRE(wf != nullptr);
}

void test_WorkflowFactory_createRestoreWorkflow() {
    WorkflowFactory factory;
    auto wf = factory.createRestoreWorkflow(nullptr);
    REQUIRE(wf != nullptr);
}

void test_WorkflowFactory_createVerificationWorkflow() {
    WorkflowFactory factory;
    auto wf = factory.createVerificationWorkflow(nullptr);
    REQUIRE(wf != nullptr);
}

void test_WorkflowFactory_createRecoveryWorkflow() {
    WorkflowFactory factory;
    auto wf = factory.createRecoveryWorkflow(nullptr);
    REQUIRE(wf != nullptr);
}

void test_WorkflowFactory_createCustomWorkflow() {
    WorkflowFactory factory;
    WorkflowBuilder builder;
    builder.connect().detect();
    auto wf = factory.createCustomWorkflow(nullptr, std::move(builder));
    REQUIRE(wf != nullptr);
}

// ───12 — VirtualWorkflowRuntime ───────────────────────────────────────

void test_VirtualRuntime_createStep() {
    VirtualWorkflowRuntime rt;
    auto step = rt.createStep("test", WorkflowStepType::Connect);
    REQUIRE(step != nullptr);
    REQUIRE(step->name() == "test");
    REQUIRE(step->type() == WorkflowStepType::Connect);
}

void test_VirtualRuntime_buildFlashWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    auto count = rt.buildFlashWorkflow(*engine);
    REQUIRE(count == 7u);
    auto result = engine->run();
    REQUIRE(result.isOk());
}

void test_VirtualRuntime_buildBackupWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    auto count = rt.buildBackupWorkflow(*engine);
    REQUIRE(count == 3u);
    auto result = engine->run();
    REQUIRE(result.isOk());
}

void test_VirtualRuntime_buildRestoreWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    auto count = rt.buildRestoreWorkflow(*engine, "userdata");
    REQUIRE(count == 3u);
}

void test_VirtualRuntime_buildRecoveryWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    auto count = rt.buildRecoveryWorkflow(*engine);
    REQUIRE(count == 3u);
    auto result = engine->run();
    REQUIRE(result.isError());
}

void test_VirtualRuntime_buildCancelWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    rt.buildCancelWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::Cancelled);
}

void test_VirtualRuntime_buildRollbackWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    rt.buildRollbackWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
}

void test_VirtualRuntime_buildLargeWorkflow() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    auto count = rt.buildLargeWorkflow(*engine, 5);
    REQUIRE(count == 15u);
}

void test_VirtualRuntime_createFirmwarePackage() {
    VirtualWorkflowRuntime rt;
    auto pkg = rt.createFirmwarePackage();
    REQUIRE(pkg != nullptr);
}

void test_VirtualRuntime_getStep() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    rt.buildFlashWorkflow(*engine);
    auto* step = rt.getStep("flash");
    REQUIRE(step != nullptr);
    auto* missing = rt.getStep("nonexistent");
    REQUIRE(missing == nullptr);
}

// ───13 — Stress & Integration ─────────────────────────────────────────

void test_Rollback() {
    auto engine = createEngineWithSteps(2);
    auto step = std::make_unique<VirtualStep>(
        "fail_step", WorkflowStepType::Flash,
        ErrorCode::JobFailed);
    engine->addStep(std::move(step));
    auto result = engine->run();
    REQUIRE(result.isError());
    auto rbResult = engine->rollback();
    REQUIRE(rbResult.isOk());
    REQUIRE(engine->state() == WorkflowState::Ready);
}

void test_Retry() {
    WorkflowEngine engine;
    WorkflowOptions opts;
    opts.retryCount = 2;
    engine.setOptions(opts);

    int attempts = 0;
    auto step = std::make_unique<VirtualStep>(
        "retry_step", WorkflowStepType::Connect);
    step->setError(ErrorCode::TransportError);
    engine.addStep(std::move(step));

    auto result = engine.run();
    REQUIRE(result.isError());
}

void test_PauseResume() {
    auto engine = createEngineWithSteps(5);
    engine->setOptions(WorkflowOptions{});

    std::thread t([&]() {
        {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (engine->state() != WorkflowState::Running && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        engine->pause();
        {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (engine->state() != WorkflowState::Paused && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        engine->resume();
    });

    auto result = engine->run();
    t.join();

    REQUIRE((result.isOk() || result.isError()));
    if (result.isOk()) {
        REQUIRE(engine->state() == WorkflowState::Completed);
    }
}

void test_CancelDuringExecution() {
    auto engine = createEngineWithSteps(10);
    engine->setOptions(WorkflowOptions{});

    std::thread t([&]() {
        {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (engine->state() != WorkflowState::Running && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        engine->cancel();
    });

    auto result = engine->run();
    t.join();

    if (result.isError()) {
        REQUIRE(result.error() == ErrorCode::Cancelled);
    }
}

void test_RecoveryAfterFailure() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    WorkflowOptions opts;
    opts.retryCount = 3;
    opts.autoRollback = true;
    engine->setOptions(opts);
    rt.buildRecoveryWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
}

void test_WorkflowHistoryRoundTrip() {
    auto engine = createEngineWithSteps(2);
    engine->run();
    auto history = engine->history();
    REQUIRE(history.size() == 2u);
}

void test_ProgressCallbacks() {
    WorkflowEngine engine;
    int progressCalls = 0;
    engine.setProgressCallback([&](const WorkflowProgress&) { ++progressCalls; });
    engine.addStep(std::make_unique<VirtualStep>("a", WorkflowStepType::Connect));
    engine.run();
    REQUIRE(progressCalls > 0);
}

void test_StateCallbacks() {
    WorkflowEngine engine;
    WorkflowState lastState = WorkflowState::Created;
    engine.setStateCallback([&](WorkflowState, WorkflowState newState) {
        lastState = newState;
    });
    engine.addStep(std::make_unique<VirtualStep>("a", WorkflowStepType::Connect));
    engine.run();
    REQUIRE(lastState == WorkflowState::Completed);
}

void test_ErrorCallbacks() {
    WorkflowEngine engine;
    bool errorCalled = false;
    engine.setErrorCallback([&](ErrorCode, const std::string&) { errorCalled = true; });
    engine.addStep(std::make_unique<VirtualStep>(
        "fail", WorkflowStepType::Flash,
        ErrorCode::JobFailed));
    engine.run();
    REQUIRE(errorCalled);
}

void test_LargeWorkflow() {
    WorkflowEngine engine;
    WorkflowOptions opts;
    opts.retryCount = 1;
    engine.setOptions(opts);
    for (int i = 0; i < 100; ++i) {
        engine.addStep(std::make_unique<VirtualStep>(
            "step_" + std::to_string(i), WorkflowStepType::Custom));
    }
    auto result = engine.run();
    REQUIRE(result.isOk());
    REQUIRE(engine.state() == WorkflowState::Completed);
}

void test_EmptyWorkflow() {
    WorkflowEngine engine;
    auto result = engine.run();
    REQUIRE(result.isOk());
    REQUIRE(engine.state() == WorkflowState::Completed);
}

void test_StepWithCustomResult() {
    WorkflowEngine engine;
    engine.addStep(std::make_unique<VirtualStep>(
        "custom", WorkflowStepType::Custom,
        ErrorCode::JobFailed));
    auto result = engine.run();
    REQUIRE(result.isError());
}

void test_WorkflowOptionsContinueOnWarning() {
    WorkflowEngine engine;
    WorkflowOptions opts;
    opts.continueOnWarning = true;
    opts.retryCount = 0;
    engine.setOptions(opts);

    engine.addStep(std::make_unique<VirtualStep>("a", WorkflowStepType::Connect));
    engine.addStep(std::make_unique<VirtualStep>(
        "b", WorkflowStepType::Flash,
        ErrorCode::JobFailed));
    engine.addStep(std::make_unique<VirtualStep>("c", WorkflowStepType::Disconnect));

    auto result = engine.run();
    REQUIRE(result.isError());
}

void test_WorkflowEngineMultipleRuns() {
    auto engine = createEngineWithSteps(3);
    engine->run();
    REQUIRE(engine->state() == WorkflowState::Completed);
    engine->reset();
    engine->addStep(std::make_unique<VirtualStep>("new", WorkflowStepType::Custom));
    auto result = engine->run();
    REQUIRE(result.isOk());
}

// ─── Failure Injection ──────────────────────────────────────────────────────────

void test_FailureInjection_loaderFailure() {
    VirtualWorkflowRuntime rt;
    rt.config().simulateLoaderFailure = true;
    auto engine = rt.createWorkflow();
    rt.buildFlashWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
}

void test_FailureInjection_verificationFailure() {
    VirtualWorkflowRuntime rt;
    rt.config().simulateVerificationFailure = true;
    auto engine = rt.createWorkflow();
    rt.buildFlashWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
}

void test_FailureInjection_cancelScenario() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    rt.buildCancelWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::Cancelled);
}

void test_FailureInjection_rollbackScenario() {
    VirtualWorkflowRuntime rt;
    auto engine = rt.createWorkflow();
    rt.buildRollbackWorkflow(*engine);
    auto result = engine->run();
    REQUIRE(result.isError());
    auto rbResult = engine->rollback();
    REQUIRE(rbResult.isOk());
}

