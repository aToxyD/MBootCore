#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareResolver.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <string>
#include <vector>

namespace mbootcore {
namespace firmware {

class FlashPlanGenerator {
public:
    FlashPlanGenerator() = default;

    Result<FlashPlan> generatePlan(const FirmwarePackage& pkg);
    Result<FlashPlan> generatePlanForDevice(const FirmwarePackage& pkg,
                                             const discovery::DeviceDescriptor& device);
    
    FlashPlan generateFromResolved(const ResolvedPackage& resolved);

private:
    void addProgrammerStep(const FirmwarePackage& pkg, FlashPlan& plan);
    void addGptStep(const FirmwarePackage& pkg, FlashPlan& plan);
    void addImageSteps(const FirmwarePackage& pkg, FlashPlan& plan);
    void addVerifySteps(const FirmwarePackage& pkg, FlashPlan& plan);
    void addRebootStep(FlashPlan& plan);
};

} // namespace firmware
} // namespace mbootcore
