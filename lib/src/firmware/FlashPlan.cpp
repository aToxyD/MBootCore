#include <mbootcore/firmware/FlashPlan.hpp>
#include <algorithm>
#include <set>

namespace mbootcore {
namespace firmware {

Result<FlashPlan> FlashPlanGenerator::generatePlan(const FirmwarePackage& pkg) {
    FlashPlan plan;

    // Check if programmer is needed
    auto progResult = pkg.getImageByType(ImageType::Programmer);
    if (progResult.isOk()) {
        plan.requireProgrammer = true;
        addProgrammerStep(pkg, plan);
    }

    // Check if GPT update is needed
    auto gptResult = pkg.getImageByType(ImageType::GPT);
    if (gptResult.isOk()) {
        plan.requireGptUpdate = true;
        addGptStep(pkg, plan);
    }

    // Add steps for all non-GPT, non-programmer images
    for (const auto& img : pkg.images()) {
        if (img.info.type == ImageType::Programmer || img.info.type == ImageType::GPT) {
            continue;
        }
        
        FlashStep step;
        step.type = FlashStepType::FlashPartition;
        step.partitionName = img.info.partitionName.empty() ? img.info.name : img.info.partitionName;
        step.imageName = img.info.name;
        step.offset = img.info.offset;
        step.size = img.info.size;
        step.data = img.data;
        step.description = "Flash " + step.partitionName;
        step.requireBackup = true;
        
        plan.steps.push_back(std::move(step));
        plan.totalBytes += img.info.size;
    }

    // Add verify steps
    addVerifySteps(pkg, plan);

    plan.estimatedTimeMs = plan.totalBytes / 1000; // rough estimate: 1MB/s
    
    return plan;
}

Result<FlashPlan> FlashPlanGenerator::generatePlanForDevice(
    const FirmwarePackage& pkg,
    const discovery::DeviceDescriptor& device) {
    (void)device;
    return generatePlan(pkg);
}

FlashPlan FlashPlanGenerator::generateFromResolved(const ResolvedPackage& resolved) {
    FlashPlan plan;

    if (resolved.programmerFound) {
        plan.requireProgrammer = true;
        FlashStep step;
        step.type = FlashStepType::FlashProgrammer;
        step.description = "Upload programmer";
        step.imageName = "programmer";
        step.requireBackup = false;
        plan.steps.push_back(std::move(step));
    }

    if (resolved.gptFound) {
        plan.requireGptUpdate = true;
        FlashStep step;
        step.type = FlashStepType::UpdateGPT;
        step.description = "Update GPT";
        step.requireBackup = true;
        plan.steps.push_back(std::move(step));
    }

    if (resolved.package) {
        for (const auto& img : resolved.package->images()) {
            if (img.info.type == ImageType::Programmer || img.info.type == ImageType::GPT) {
                continue;
            }
            
            FlashStep step;
            step.type = FlashStepType::FlashPartition;
            step.partitionName = img.info.partitionName.empty() ? img.info.name : img.info.partitionName;
            step.imageName = img.info.name;
            step.size = img.info.size;
            step.data = img.data;
            step.description = "Flash " + step.partitionName;
            step.requireBackup = true;
            
            plan.steps.push_back(std::move(step));
            plan.totalBytes += img.info.size;
        }
    }

    // Reboot
    addRebootStep(plan);
    
    plan.estimatedTimeMs = plan.totalBytes / 1000;
    return plan;
}

void FlashPlanGenerator::addProgrammerStep(const FirmwarePackage& pkg, FlashPlan& plan) {
    (void)pkg;
    FlashStep step;
    step.type = FlashStepType::FlashProgrammer;
    step.description = "Upload programmer";
    step.imageName = "programmer";
    step.requireBackup = false;
    plan.steps.insert(plan.steps.begin(), std::move(step));
}

void FlashPlanGenerator::addGptStep(const FirmwarePackage& pkg, FlashPlan& plan) {
    (void)pkg;
    FlashStep step;
    step.type = FlashStepType::UpdateGPT;
    step.description = "Update GPT";
    step.requireBackup = true;
    plan.steps.insert(plan.steps.begin() + (plan.requireProgrammer ? 1 : 0), std::move(step));
}

void FlashPlanGenerator::addImageSteps(const FirmwarePackage& pkg, FlashPlan& plan) {
    for (const auto& img : pkg.images()) {
        if (img.info.type == ImageType::Programmer || img.info.type == ImageType::GPT) {
            continue;
        }
        
        FlashStep step;
        step.type = FlashStepType::FlashPartition;
        step.partitionName = img.info.partitionName.empty() ? img.info.name : img.info.partitionName;
        step.imageName = img.info.name;
        step.size = img.info.size;
        step.data = img.data;
        step.description = "Flash " + step.partitionName;
        step.requireBackup = true;
        
        plan.steps.push_back(std::move(step));
        plan.totalBytes += img.info.size;
    }
}

void FlashPlanGenerator::addVerifySteps(const FirmwarePackage& pkg, FlashPlan& plan) {
    for (const auto& img : pkg.images()) {
        if (img.info.type == ImageType::Programmer || img.info.type == ImageType::GPT) {
            continue;
        }
        
        FlashStep step;
        step.type = FlashStepType::VerifyPartition;
        step.partitionName = img.info.partitionName.empty() ? img.info.name : img.info.partitionName;
        step.imageName = img.info.name;
        step.size = img.info.size;
        step.data = img.data;
        step.description = "Verify " + step.partitionName;
        step.optional = true;
        step.requireBackup = false;
        
        plan.steps.push_back(std::move(step));
    }
}

void FlashPlanGenerator::addRebootStep(FlashPlan& plan) {
    FlashStep step;
    step.type = FlashStepType::Reboot;
    step.description = "Reboot device";
    step.requireBackup = false;
    plan.steps.push_back(std::move(step));
}

} // namespace firmware
} // namespace mbootcore
