#pragma once

#include <mbootcore/workflow/WorkflowTypes.hpp>
#include <mbootcore/workflow/IWorkflow.hpp>
#include <mbootcore/workflow/WorkflowBuilder.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/session/DeviceSession.hpp>

#include <memory>

namespace mbootcore {
namespace workflow {

class WorkflowFactory {
public:
    WorkflowFactory() = default;

    std::unique_ptr<IWorkflow> createFlashWorkflow(
        session::DeviceSession* session,
        firmware::FirmwarePackage* package);

    std::unique_ptr<IWorkflow> createBackupWorkflow(
        session::DeviceSession* session,
        const std::string& partition = {});

    std::unique_ptr<IWorkflow> createRestoreWorkflow(
        session::DeviceSession* session,
        const std::string& partition = {});

    std::unique_ptr<IWorkflow> createVerificationWorkflow(
        session::DeviceSession* session);

    std::unique_ptr<IWorkflow> createRecoveryWorkflow(
        session::DeviceSession* session);

    std::unique_ptr<IWorkflow> createCustomWorkflow(
        session::DeviceSession* session,
        WorkflowBuilder builder);
};

} // namespace workflow
} // namespace mbootcore
