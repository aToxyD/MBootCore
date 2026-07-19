#pragma once

#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/job/RecoveryPolicies.hpp>

#include <memory>
#include <string>

namespace mbootcore {
namespace job {

class IJob {
public:
    virtual ~IJob() = default;

    virtual std::string id() const noexcept = 0;
    virtual JobType type() const noexcept = 0;
    virtual JobState state() const noexcept = 0;

    virtual Result<void> prepare(JobContext& context) = 0;
    virtual Result<void> execute(JobContext& context) = 0;
    virtual Result<void> rollback(JobContext& context) = 0;
    virtual Result<void> cancel() noexcept = 0;

    virtual ProgressInfo progress() const noexcept = 0;
    virtual JobResult result() const noexcept = 0;

    virtual void setRecoveryPolicy(std::unique_ptr<RecoveryPolicy> policy) = 0;
    virtual RecoveryPolicy* recoveryPolicy() const noexcept = 0;

    virtual void setConfig(const JobConfig& config) = 0;
    virtual const JobConfig& config() const noexcept = 0;

    virtual bool canRollback() const noexcept = 0;
};

} // namespace job
} // namespace mbootcore
