#pragma once

#include <mbootcore/job/IJob.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/generic/ProgressInfo.hpp>
#include <mbootcore/domain/ILogger.hpp>

#include <string>
#include <atomic>
#include <memory>
#include <chrono>

namespace mbootcore {
namespace job {

class BaseJob : public IJob {
public:
    BaseJob(const std::string& id, JobType type);
    ~BaseJob() override = default;

    std::string id() const noexcept override { return m_id; }
    JobType type() const noexcept override { return m_type; }
    JobState state() const noexcept override { return m_state; }

    Result<void> cancel() noexcept override;

    ProgressInfo progress() const noexcept override { return m_progress; }
    JobResult result() const noexcept override { return m_result; }

    void setRecoveryPolicy(std::unique_ptr<RecoveryPolicy> policy) override {
        m_recoveryPolicy = std::move(policy);
    }
    RecoveryPolicy* recoveryPolicy() const noexcept override { return m_recoveryPolicy.get(); }

    void setConfig(const JobConfig& config) override { m_config = config; }
    const JobConfig& config() const noexcept override { return m_config; }

    bool canRollback() const noexcept override;

protected:
    void setState(JobState newState) noexcept { m_state = newState; }
    void updateProgress(uint64_t transferred, uint64_t total, const std::string& op);
    void markCompleted();
    void markFailed(ErrorCode error, const std::string& message = {});
    void startTimer();
    void stopTimer();

    bool checkCancelled() const noexcept {
        return m_cancelled.load();
    }

    std::string m_id;
    JobType m_type;
    std::atomic<JobState> m_state{JobState::Pending};
    std::unique_ptr<RecoveryPolicy> m_recoveryPolicy;
    JobConfig m_config;
    ProgressInfo m_progress;
    JobResult m_result;
    std::atomic<bool> m_cancelled{false};
    std::chrono::steady_clock::time_point m_startTime;
};

class FlashJob : public BaseJob {
public:
    explicit FlashJob(const std::string& id,
                      const std::string& partitionName,
                      const ByteBuffer& data);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;
};

class BackupJob : public BaseJob {
public:
    explicit BackupJob(const std::string& id,
                       const std::string& partitionName);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

    ByteBuffer backupData() const { return m_backupData; }

private:
    ByteBuffer m_backupData;
};

class RestoreJob : public BaseJob {
public:
    explicit RestoreJob(const std::string& id,
                        const std::string& partitionName,
                        ByteBuffer data);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

private:
    ByteBuffer m_restoreData;
    ByteBuffer m_originalData;
};

class ReadJob : public BaseJob {
public:
    explicit ReadJob(const std::string& id,
                     const std::string& partitionName);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

    ByteBuffer readData() const { return m_readData; }

private:
    ByteBuffer m_readData;
};

class EraseJob : public BaseJob {
public:
    explicit EraseJob(const std::string& id,
                      const std::string& partitionName);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

private:
    ByteBuffer m_originalData;
    bool m_backedUp{false};
};

class VerifyJob : public BaseJob {
public:
    explicit VerifyJob(const std::string& id,
                       const std::string& partitionName,
                       ByteBuffer expectedData);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

    bool isVerified() const { return m_verified; }

private:
    ByteBuffer m_expectedData;
    bool m_verified{false};
};

class GPTUpdateJob : public BaseJob {
public:
    explicit GPTUpdateJob(const std::string& id);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

private:
    ByteBuffer m_originalPrimary;
    ByteBuffer m_originalBackup;
    bool m_backedUp{false};
};

class ProgrammerUploadJob : public BaseJob {
public:
    explicit ProgrammerUploadJob(const std::string& id,
                                 ByteBuffer programmerData);

    Result<void> prepare(JobContext& context) override;
    Result<void> execute(JobContext& context) override;
    Result<void> rollback(JobContext& context) override;

private:
    ByteBuffer m_programmerData;
};

} // namespace job
} // namespace mbootcore
