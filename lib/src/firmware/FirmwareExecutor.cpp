#include <mbootcore/firmware/FirmwareExecutor.hpp>
#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/gpt/PartitionManager.hpp>
#include <mbootcore/domain/ILogger.hpp>

namespace mbootcore {
namespace firmware {

Result<void> FirmwareExecutor::executePlan(FlashPlan& plan,
                                            job::JobPipeline& pipeline,
                                            job::JobContext& context) {
    m_createdJobs = 0;
    m_failedSteps = 0;

    if (context.isCancelled()) {
        return ErrorCode::Cancelled;
    }

    for (auto& step : plan.steps) {
        if (context.isCancelled()) {
            return ErrorCode::Cancelled;
        }

        if (m_progressCb) {
            m_progressCb(step.description,
                        static_cast<float>(m_createdJobs) / std::max(plan.steps.size(), size_t(1)));
        }

        auto result = createJobForStep(step, pipeline, context);
        if (!result.isOk()) {
            m_failedSteps++;
            if (!step.optional) {
                return result;
            }
        }
        m_createdJobs++;
    }

    return {};
}

Result<void> FirmwareExecutor::executePackage(FirmwarePackage& pkg,
                                               job::JobPipeline& pipeline,
                                               job::JobContext& context) {
    FlashPlanGenerator gen;
    auto planResult = gen.generatePlan(pkg);
    if (!planResult.isOk()) {
        return planResult.error();
    }
    return executePlan(planResult.value(), pipeline, context);
}

Result<void> FirmwareExecutor::createJobForStep(const FlashStep& step,
                                                 job::JobPipeline& pipeline,
                                                 job::JobContext& context) {
    switch (step.type) {
    case FlashStepType::FlashProgrammer:
        if (context.logger) {
            context.logger->info("FirmwareExecutor", "Creating programmer upload job");
        }
        // ProgrammerUploadJob requires programmer data
        // For now, create a no-op custom step
        return {};

    case FlashStepType::UpdateGPT:
        if (context.logger) {
            context.logger->info("FirmwareExecutor", "Creating GPT update job");
        }
        // GPTUpdateJob
        if (context.flashDevice) {
            pipeline.addJob(std::make_unique<job::GPTUpdateJob>("fw_gpt_update"));
            return {};
        }
        return static_cast<ErrorCode>(FirmwareError::InvalidFormat);

    case FlashStepType::FlashPartition:
        if (context.logger) {
            context.logger->info("FirmwareExecutor", "Creating flash job for " + step.partitionName);
        }
        if (context.flashDevice && !step.data.empty()) {
            pipeline.addJob(std::make_unique<job::FlashJob>(
                "fw_flash_" + step.partitionName,
                step.partitionName,
                step.data));
            return {};
        }
        return static_cast<ErrorCode>(FirmwareError::InvalidFormat);

    case FlashStepType::VerifyPartition:
        if (context.logger) {
            context.logger->info("FirmwareExecutor", "Creating verify job for " + step.partitionName);
        }
        if (context.flashDevice && !step.data.empty()) {
            pipeline.addJob(std::make_unique<job::VerifyJob>(
                "fw_verify_" + step.partitionName,
                step.partitionName,
                step.data));
            return {};
        }
        return {}; // Optional step: skip silently

    case FlashStepType::ErasePartition:
        if (context.logger) {
            context.logger->info("FirmwareExecutor", "Creating erase job for " + step.partitionName);
        }
        if (context.flashDevice) {
            pipeline.addJob(std::make_unique<job::EraseJob>(
                "fw_erase_" + step.partitionName,
                step.partitionName));
            return {};
        }
        return static_cast<ErrorCode>(FirmwareError::InvalidFormat);

    case FlashStepType::Reboot:
        if (context.logger) {
            context.logger->info("FirmwareExecutor", "Skipping reboot step (would reset device)");
        }
        return {};

    default:
        return {};
    }
}

} // namespace firmware
} // namespace mbootcore
