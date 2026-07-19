#include <mbootcore/workflow/WorkflowFactory.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/workflow/WorkflowBuilder.hpp>

namespace mbootcore {
namespace workflow {

std::unique_ptr<IWorkflow> WorkflowFactory::createFlashWorkflow(
    session::DeviceSession* session,
    firmware::FirmwarePackage* package) {

    WorkflowBuilder builder;
    builder
        .connect()
        .detect()
        .negotiate()
        .uploadLoader()
        .flash()
        .verify()
        .reboot();

    WorkflowContext ctx;
    ctx.session = session;
    ctx.firmwarePackage = package;

    builder.setContext(std::move(ctx));
    return builder.build();
}

std::unique_ptr<IWorkflow> WorkflowFactory::createBackupWorkflow(
    session::DeviceSession* session,
    const std::string& partition) {

    WorkflowBuilder builder;
    builder
        .connect()
        .detect()
        .backup(partition);

    WorkflowContext ctx;
    ctx.session = session;
    builder.setContext(std::move(ctx));
    return builder.build();
}

std::unique_ptr<IWorkflow> WorkflowFactory::createRestoreWorkflow(
    session::DeviceSession* session,
    const std::string& partition) {

    WorkflowBuilder builder;
    builder
        .connect()
        .detect()
        .restore(partition);

    WorkflowContext ctx;
    ctx.session = session;
    builder.setContext(std::move(ctx));
    return builder.build();
}

std::unique_ptr<IWorkflow> WorkflowFactory::createVerificationWorkflow(
    session::DeviceSession* session) {

    WorkflowBuilder builder;
    builder
        .connect()
        .detect()
        .verify();

    WorkflowContext ctx;
    ctx.session = session;
    builder.setContext(std::move(ctx));
    return builder.build();
}

std::unique_ptr<IWorkflow> WorkflowFactory::createRecoveryWorkflow(
    session::DeviceSession* session) {

    WorkflowBuilder builder;
    builder
        .connect()
        .detect()
        .negotiate()
        .uploadLoader();

    WorkflowContext ctx;
    ctx.session = session;
    builder.setContext(std::move(ctx));
    return builder.build();
}

std::unique_ptr<IWorkflow> WorkflowFactory::createCustomWorkflow(
    session::DeviceSession* session,
    WorkflowBuilder builder) {

    WorkflowContext ctx;
    ctx.session = session;
    builder.setContext(std::move(ctx));
    return builder.build();
}

} // namespace workflow
} // namespace mbootcore
