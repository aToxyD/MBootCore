#include <mbootcore/workflow/WorkflowExecutor.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/job/JobPipeline.hpp>
#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/firmware/FlashPlan.hpp>
#include <mbootcore/firmware/FirmwareExecutor.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/session/DeviceSession.hpp>
#include <mbootcore/loader/LoaderFramework.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>

namespace mbootcore {
namespace workflow {

Result<void> WorkflowExecutor::executeConnect(WorkflowContext& context) {
    if (!context.session) {
        ++m_failedSteps;
        return ErrorCode::SessionNotConnected;
    }

    auto result = context.session->connect();
    if (result) {
        ++m_executedSteps;
    } else {
        ++m_failedSteps;
    }
    return result;
}

Result<void> WorkflowExecutor::executeDetect(WorkflowContext& context) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceNotIdentified;
    }

    auto deviceInfo = context.session->flashDevice()->deviceInfo();
    if (deviceInfo.isError()) {
        ++m_failedSteps;
        return deviceInfo.error();
    }

    auto storageInfo = context.session->flashDevice()->getStorageInfo();
    if (storageInfo) {
        context.properties["storage_type"] = std::to_string(
            static_cast<uint32_t>(storageInfo.value().type));
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeNegotiate(WorkflowContext& context) {
    if (!context.bootPipeline) {
        return ErrorCode::InvalidArgument;
    }

    auto result = context.bootPipeline->run();
    if (result) {
        ++m_executedSteps;
    } else {
        ++m_failedSteps;
    }
    return result;
}

Result<void> WorkflowExecutor::executeUploadLoader(WorkflowContext& context) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceDisconnected;
    }

    auto* flashDevice = context.session->flashDevice();
    (void)flashDevice;
    if (context.loaderFramework) {
        auto loaders = context.loaderFramework->listLoaders();
        if (loaders.empty()) {
            ++m_failedSteps;
            return ErrorCode::LoaderNotFound;
        }
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeFlash(WorkflowContext& context) {
    if (!context.firmwarePackage || !context.jobPipeline) {
        return ErrorCode::InvalidArgument;
    }

    if (!context.flashPlanGenerator || !context.firmwareExecutor) {
        ++m_failedSteps;
        return ErrorCode::FirmwareMissingProgrammer;
    }

    MBOOT_TRY_ASSIGN(plan, context.flashPlanGenerator->generatePlan(*context.firmwarePackage));

    auto& pipeline = *context.jobPipeline;
    auto execResult = context.firmwareExecutor->executePlan(
        plan, pipeline, *context.jobContext);

    if (execResult) {
        auto plResult = pipeline.run(*context.jobContext);
        if (plResult.isError()) {
            ++m_failedSteps;
            return plResult;
        }
    } else {
        ++m_failedSteps;
        return execResult;
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeVerify(WorkflowContext& context) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceDisconnected;
    }

    auto partitions = context.session->flashDevice()->getPartitions();
    if (partitions.isError()) {
        ++m_failedSteps;
        return partitions.error();
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeGPT(WorkflowContext& context) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceDisconnected;
    }

    auto partitions = context.session->flashDevice()->getPartitions();
    if (partitions.isError()) {
        ++m_failedSteps;
        return partitions.error();
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeBackup(WorkflowContext& context,
                                               const std::string& partition) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceDisconnected;
    }

    if (partition.empty()) {
        MBOOT_TRY_ASSIGN(partitions, context.session->flashDevice()->getPartitions());
        for (const auto& p : partitions.entries) {
            auto data = context.session->readPartition(p.name);
            if (data.isError()) {
                ++m_failedSteps;
                return data.error();
            }
        }
    } else {
        auto data = context.session->readPartition(partition);
        if (data.isError()) {
            ++m_failedSteps;
            return data.error();
        }
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeRestore(WorkflowContext& context,
                                                const std::string& partition) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceDisconnected;
    }

    if (partition.empty()) {
        return ErrorCode::InvalidArgument;
    }

    auto result = context.session->writePartition(partition, ByteBuffer{});
    if (result.isError()) {
        ++m_failedSteps;
        return result;
    }

    ++m_executedSteps;
    return {};
}

Result<void> WorkflowExecutor::executeReboot(WorkflowContext& context) {
    if (!context.session || !context.session->flashDevice()) {
        return ErrorCode::DeviceDisconnected;
    }

    auto result = context.session->resetDevice();
    if (result) {
        ++m_executedSteps;
    } else {
        ++m_failedSteps;
    }
    return result;
}

Result<void> WorkflowExecutor::executeDisconnect(WorkflowContext& context) {
    if (context.session) {
        context.session->disconnect();
    }
    ++m_executedSteps;
    return {};
}

} // namespace workflow
} // namespace mbootcore
