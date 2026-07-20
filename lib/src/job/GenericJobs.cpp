#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/gpt/PartitionManager.hpp>
#include "common/NumericUtils.hpp"

namespace mbootcore {
namespace job {

BaseJob::BaseJob(const std::string& id, JobType type)
    : m_id(id)
    , m_type(type) {
    m_recoveryPolicy = std::make_unique<DefaultRecoveryPolicy>();
}

Result<void> BaseJob::cancel() noexcept {
    m_cancelled = true;
    if (m_state == JobState::Pending || m_state == JobState::Running) {
        m_state = JobState::Cancelled;
    }
    return {};
}

bool BaseJob::canRollback() const noexcept {
    return m_recoveryPolicy && m_recoveryPolicy->canRollback();
}

void BaseJob::updateProgress(uint64_t transferred, uint64_t total, const std::string& op) {
    m_progress.transferredBytes = transferred;
    m_progress.totalBytes = total;
    m_progress.percentage = total > 0 ? (100.0 * transferred / total) : 0.0;
    m_progress.currentOperation = op;
    m_progress.stage = m_id;
    if (total > 0 && transferred > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_startTime).count();
        if (elapsed > 0) {
            m_progress.speedBps = static_cast<double>(transferred) * 1000.0 / elapsed;
            double remaining = static_cast<double>(total - transferred);
            m_progress.estimatedSeconds = m_progress.speedBps > 0.0
                ? remaining / m_progress.speedBps : 0.0;
        }
    }
}

void BaseJob::markCompleted() {
    m_state = JobState::Completed;
    stopTimer();
    m_result.success = true;
    m_result.error = ErrorCode::Success;
    m_result.statistics = {};
    m_result.statistics.elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_startTime);
    m_result.statistics.processedBytes = m_progress.transferredBytes;
    m_result.statistics.averageSpeedBps = m_progress.speedBps;
}

void BaseJob::markFailed(ErrorCode error, const std::string& message) {
    m_state = JobState::Failed;
    stopTimer();
    m_result.success = false;
    m_result.error = error;
    m_result.errorMessage = message;
    m_result.statistics.elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_startTime);
    m_result.statistics.processedBytes = m_progress.transferredBytes;
}

void BaseJob::startTimer() {
    m_startTime = std::chrono::steady_clock::now();
}

void BaseJob::stopTimer() {
    auto now = std::chrono::steady_clock::now();
    m_result.statistics.endTime = now;
}

FlashJob::FlashJob(const std::string& id,
                   const std::string& partitionName,
                   const ByteBuffer& data)
    : BaseJob(id, JobType::Flash) {
    m_config.partitionName = partitionName;
    m_config.data = data;
}

Result<void> FlashJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    if (m_config.data.empty()) {
        return ErrorCode::InvalidArgument;
    }
    setState(JobState::Preparing);
    updateProgress(0, m_config.data.size(), "Preparing flash");
    return {};
}

Result<void> FlashJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    if (context.logger) {
        context.logger->info("FlashJob", "Flashing partition: " + m_config.partitionName);
    }

    if (!m_config.partitionName.empty()) {
        auto result = context.flashDevice->writePartition(m_config.partitionName, m_config.data);
        if (!result) {
            markFailed(result.error(), "Flash write failed");
            return result;
        }
    }

    updateProgress(m_config.data.size(), m_config.data.size(), "Flash complete");
    markCompleted();
    return {};
}

Result<void> FlashJob::rollback(JobContext& context) {
    if (context.logger) {
        context.logger->info("FlashJob", "Rolling back flash for " + m_config.partitionName);
    }
    setState(JobState::RollingBack);
    return {};
}

BackupJob::BackupJob(const std::string& id,
                     const std::string& partitionName)
    : BaseJob(id, JobType::Backup) {
    m_config.partitionName = partitionName;
}

Result<void> BackupJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> BackupJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    if (context.logger) {
        context.logger->info("BackupJob", "Backing up partition: " + m_config.partitionName);
    }

    if (context.partitionManager) {
        auto result = context.partitionManager->backupPartition(m_config.partitionName);
        if (!result) {
            markFailed(result.error(), "Backup failed");
            return result.error();
        }
        m_backupData = std::move(result.value());
    } else {
        auto result = context.flashDevice->readPartition(m_config.partitionName);
        if (!result) {
            markFailed(result.error(), "Backup read failed");
            return result.error();
        }
        m_backupData = std::move(result.value());
    }

    updateProgress(m_backupData.size(), m_backupData.size(), "Backup complete");
    markCompleted();
    return {};
}

Result<void> BackupJob::rollback(JobContext& context) {
    (void)context;
    setState(JobState::RollingBack);
    return {};
}

RestoreJob::RestoreJob(const std::string& id,
                       const std::string& partitionName,
                       ByteBuffer data)
    : BaseJob(id, JobType::Restore) {
    m_config.partitionName = partitionName;
    m_restoreData = std::move(data);
}

Result<void> RestoreJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    if (m_restoreData.empty()) {
        return ErrorCode::InvalidArgument;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> RestoreJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    auto readResult = context.flashDevice->readPartition(m_config.partitionName);
    if (readResult) {
        m_originalData = std::move(readResult.value());
    }

    if (context.logger) {
        context.logger->info("RestoreJob", "Restoring partition: " + m_config.partitionName);
    }

    if (context.partitionManager) {
        auto result = context.partitionManager->restorePartition(m_config.partitionName, m_restoreData);
        if (!result) {
            markFailed(result.error(), "Restore failed");
            return result.error();
        }
    } else {
        auto result = context.flashDevice->writePartition(m_config.partitionName, m_restoreData);
        if (!result) {
            markFailed(result.error(), "Restore write failed");
            return result.error();
        }
    }

    updateProgress(m_restoreData.size(), m_restoreData.size(), "Restore complete");
    markCompleted();
    return {};
}

Result<void> RestoreJob::rollback(JobContext& context) {
    if (!m_originalData.empty()) {
        setState(JobState::RollingBack);
        if (context.partitionManager) {
            return context.partitionManager->restorePartition(m_config.partitionName, m_originalData);
        }
        return context.flashDevice->writePartition(m_config.partitionName, m_originalData);
    }
    return {};
}

ReadJob::ReadJob(const std::string& id,
                 const std::string& partitionName)
    : BaseJob(id, JobType::Read) {
    m_config.partitionName = partitionName;
}

Result<void> ReadJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> ReadJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    if (context.logger) {
        context.logger->info("ReadJob", "Reading partition: " + m_config.partitionName);
    }

    auto result = context.flashDevice->readPartition(m_config.partitionName);
    if (!result) {
        markFailed(result.error(), "Read failed");
        return result.error();
    }

    m_readData = std::move(result.value());
    updateProgress(m_readData.size(), m_readData.size(), "Read complete");
    markCompleted();
    return {};
}

Result<void> ReadJob::rollback(JobContext& context) {
    (void)context;
    setState(JobState::RollingBack);
    return {};
}

EraseJob::EraseJob(const std::string& id,
                   const std::string& partitionName)
    : BaseJob(id, JobType::Erase) {
    m_config.partitionName = partitionName;
}

Result<void> EraseJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> EraseJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    auto readResult = context.flashDevice->readPartition(m_config.partitionName);
    if (readResult) {
        m_originalData = std::move(readResult.value());
        m_backedUp = true;
    }

    if (context.logger) {
        context.logger->info("EraseJob", "Erasing partition: " + m_config.partitionName);
    }

    auto result = context.flashDevice->erasePartition(m_config.partitionName);
    if (!result) {
        markFailed(result.error(), "Erase failed");
        return result.error();
    }

    updateProgress(1, 1, "Erase complete");
    markCompleted();
    return {};
}

Result<void> EraseJob::rollback(JobContext& context) {
    if (m_backedUp && !m_originalData.empty()) {
        setState(JobState::RollingBack);
        return context.flashDevice->writePartition(m_config.partitionName, m_originalData);
    }
    return {};
}

VerifyJob::VerifyJob(const std::string& id,
                     const std::string& partitionName,
                     ByteBuffer expectedData)
    : BaseJob(id, JobType::Verify) {
    m_config.partitionName = partitionName;
    m_expectedData = std::move(expectedData);
}

Result<void> VerifyJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    if (m_expectedData.empty()) {
        return ErrorCode::InvalidArgument;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> VerifyJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    if (context.logger) {
        context.logger->info("VerifyJob", "Verifying partition: " + m_config.partitionName);
    }

    auto result = context.flashDevice->readPartition(m_config.partitionName);
    if (!result) {
        markFailed(result.error(), "Verify read failed");
        return result.error();
    }

    const auto& actual = result.value();
    if (actual.size() != m_expectedData.size()) {
        markFailed(ErrorCode::JobDataMismatch, "Size mismatch");
        return ErrorCode::JobDataMismatch;
    }

    m_verified = (actual == m_expectedData);
    if (!m_verified) {
        markFailed(ErrorCode::JobDataMismatch, "Data mismatch");
        return ErrorCode::JobDataMismatch;
    }

    updateProgress(m_expectedData.size(), m_expectedData.size(), "Verify complete");
    markCompleted();
    return {};
}

Result<void> VerifyJob::rollback(JobContext& context) {
    (void)context;
    return {};
}

GPTUpdateJob::GPTUpdateJob(const std::string& id)
    : BaseJob(id, JobType::GPTUpdate) {
}

Result<void> GPTUpdateJob::prepare(JobContext& context) {
    if (!context.flashDevice && !context.partitionManager) {
        return ErrorCode::JobDeviceNotReady;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> GPTUpdateJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    if (context.logger) {
        context.logger->info("GPTUpdateJob", "Updating GPT");
    }

    if (context.partitionManager) {
        auto openResult = context.partitionManager->open();
        if (!openResult.isOk()) {
            markFailed(openResult.error(), "Failed to open partition manager");
            return openResult.error();
        }
    }

    if (context.flashDevice) {
        auto storageInfo = context.flashDevice->getStorageInfo();
        if (storageInfo) {
            uint64_t sectorSize = storageInfo.value().sectorSize > 0
                                  ? storageInfo.value().sectorSize : 512;
            uint64_t gptSize = (34 + 128 * 128 / sectorSize) * sectorSize;

            auto primaryResult = context.flashDevice->readMemory(0, numeric::checked_cast<size_t>(gptSize));
            if (primaryResult) {
                m_originalPrimary = std::move(primaryResult.value());
                m_backedUp = true;
            }

            auto lastLba = storageInfo.value().numSectors - 1;
            uint64_t backupOffset = (lastLba - 33) * sectorSize;
            auto backupResult = context.flashDevice->readMemory(backupOffset, numeric::checked_cast<size_t>(gptSize));
            if (backupResult.isOk()) {
                m_originalBackup = std::move(backupResult.value());
            }
        }
    }

    if (context.partitionManager) {
        auto refreshResult = context.partitionManager->refreshTable();
        if (!refreshResult.isOk()) {
            markFailed(refreshResult.error(), "GPT refresh failed");
            return refreshResult.error();
        }
    }

    updateProgress(1, 1, "GPT update complete");
    markCompleted();
    return {};
}

Result<void> GPTUpdateJob::rollback(JobContext& context) {
    if (m_backedUp && !m_originalPrimary.empty() && context.flashDevice) {
        setState(JobState::RollingBack);
        return context.flashDevice->writeMemory(0, m_originalPrimary);
    }
    return {};
}

ProgrammerUploadJob::ProgrammerUploadJob(const std::string& id,
                                         ByteBuffer programmerData)
    : BaseJob(id, JobType::ProgrammerUpload) {
    m_programmerData = std::move(programmerData);
}

Result<void> ProgrammerUploadJob::prepare(JobContext& context) {
    if (!context.flashDevice) {
        return ErrorCode::JobDeviceNotReady;
    }
    if (m_programmerData.empty()) {
        return ErrorCode::InvalidArgument;
    }
    setState(JobState::Preparing);
    return {};
}

Result<void> ProgrammerUploadJob::execute(JobContext& context) {
    setState(JobState::Running);
    startTimer();

    if (checkCancelled()) return ErrorCode::Cancelled;

    if (context.logger) {
        context.logger->info("ProgrammerUploadJob", "Uploading programmer");
    }

    auto result = context.flashDevice->uploadLoader(m_programmerData);
    if (!result) {
        markFailed(result.error(), "Programmer upload failed");
        return result.error();
    }

    updateProgress(m_programmerData.size(), m_programmerData.size(), "Upload complete");
    markCompleted();
    return {};
}

Result<void> ProgrammerUploadJob::rollback(JobContext& context) {
    (void)context;
    return {};
}

} // namespace job
} // namespace mbootcore
