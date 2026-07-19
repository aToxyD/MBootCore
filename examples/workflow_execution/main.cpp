#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/workflow/WorkflowBuilder.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/IWorkflowStep.hpp>
#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>

using namespace mbootcore;

class LogStep : public workflow::IWorkflowStep {
public:
    explicit LogStep(std::string name, std::string msg)
        : m_name(std::move(name)), m_msg(std::move(msg)) {}

    Result<void> prepare(workflow::WorkflowContext& ctx) override {
        log("prepare", ctx);
        return {};
    }

    Result<void> execute(workflow::WorkflowContext& ctx) override {
        log("execute", ctx);
        m_progress = 1.0;
        return {};
    }

    Result<void> rollback(workflow::WorkflowContext& ctx) override {
        log("rollback", ctx);
        return {};
    }

    std::string name() const noexcept override { return m_name; }
    workflow::WorkflowStepType type() const noexcept override {
        return workflow::WorkflowStepType::Custom;
    }
    double progress() const noexcept override { return m_progress; }

private:
    void log(const std::string& action, workflow::WorkflowContext& ctx) {
        if (ctx.isCancelled()) return;
        std::cout << "[Workflow] " << m_name << " " << action
                  << ": " << m_msg << std::endl;
    }

    std::string m_name;
    std::string m_msg;
    double m_progress{0.0};
};

int main() {
    auto logger = std::make_shared<ConsoleLogger>();
    logger->info("example", "Workflow Execution Example\n");

    // Build a custom workflow using WorkflowBuilder
    workflow::WorkflowBuilder builder;
    builder
        .connect(3)
        .detect()
        .negotiate()
        .custom(std::make_unique<LogStep>("validate", "Checking device state"))
        .custom(std::make_unique<LogStep>("prepare", "Preparing flash environment"))
        .flash()
        .verify()
        .custom(std::make_unique<LogStep>("finalize", "Finalizing device"))
        .reboot()
        .disconnect();

    builder.setOptions({
        true,
        false,
        2,
        std::chrono::seconds(30),
        false,
    });

    logger->info("example",
                 "Workflow built with " + std::to_string(builder.stepCount()) + " steps");

    // Build the workflow
    auto workflow = builder.build();
    if (!workflow) {
        logger->error("example", "Failed to build workflow");
        return EXIT_FAILURE;
    }

    logger->info("example", "Workflow state: " + workflow->stateString());

    // Set up progress callback
    workflow->setContext({});

    // Run the workflow
    logger->info("example", "\n--- Starting workflow execution ---\n");

    auto prepareResult = workflow->prepare();
    if (!prepareResult.isOk()) {
        logger->error("example",
                      "Workflow prepare failed: " + std::string(toString(prepareResult.error())));
        return EXIT_FAILURE;
    }
    logger->info("example", "Workflow prepared");

    auto runResult = workflow->run();
    if (runResult.isOk()) {
        logger->info("example", "\nWorkflow completed successfully");
    } else {
        logger->error("example",
                      "\nWorkflow failed: " + std::string(toString(runResult.error())));
    }

    // Inspect results
    auto progress = workflow->progress();
    logger->info("example",
                 "Final progress: " + std::to_string(static_cast<int>(progress.overallProgress * 100)) +
                     "% - " + progress.currentStep);

    auto stats = workflow->statistics();
    logger->info("example",
                 "Elapsed: " + std::to_string(stats.elapsed.count()) + "ms, " +
                     "Retries: " + std::to_string(stats.retries) + ", " +
                     "Errors: " + std::to_string(stats.errors));

    // Rollback demonstration
    logger->info("example", "\n--- Demonstrating rollback ---\n");
    auto rollbackResult = workflow->rollback();
    if (rollbackResult.isOk()) {
        logger->info("example", "Workflow rolled back successfully");
    } else {
        logger->info("example",
                     "Rollback result: " + std::string(toString(rollbackResult.error())));
    }

    return EXIT_SUCCESS;
}
