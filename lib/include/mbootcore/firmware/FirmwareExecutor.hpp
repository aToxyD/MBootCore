#pragma once

#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FlashPlan.hpp>
#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/job/JobPipeline.hpp>

namespace mbootcore {
namespace firmware {

class FirmwareExecutor {
public:
    FirmwareExecutor() = default;
    
    Result<void> executePlan(FlashPlan& plan,
                              job::JobPipeline& pipeline,
                              job::JobContext& context);
    
    Result<void> executePackage(FirmwarePackage& pkg,
                                 job::JobPipeline& pipeline,
                                 job::JobContext& context);

    size_t createdJobs() const noexcept { return m_createdJobs; }
    size_t failedSteps() const noexcept { return m_failedSteps; }
    
    // Progress
    using ProgressCallback = std::function<void(const std::string& step, float progress)>;
    void setProgressCallback(ProgressCallback cb) { m_progressCb = std::move(cb); }

private:
    Result<void> createJobForStep(const FlashStep& step,
                                   job::JobPipeline& pipeline,
                                   job::JobContext& context);
    
    size_t m_createdJobs{0};
    size_t m_failedSteps{0};
    ProgressCallback m_progressCb;
};

} // namespace firmware
} // namespace mbootcore
