#include <catch2/catch_test_macros.hpp>

#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/job/IJob.hpp>
#include <mbootcore/job/RecoveryPolicies.hpp>
#include <mbootcore/job/JobHistory.hpp>
#include <mbootcore/job/ProgressAggregator.hpp>
#include <mbootcore/job/JobPipeline.hpp>
#include <mbootcore/job/JobScheduler.hpp>
#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/job/VirtualJobDevice.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>

#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

using namespace mbootcore;
using namespace mbootcore::job;

namespace {

class NullTestLogger : public ILogger {
public:
    void log(LogLevel, std::string_view, const std::string&) override {}
    void setLevel(LogLevel) override {}
    LogLevel level() const noexcept override { return LogLevel::None; }
};

ByteBuffer makeData(size_t size, uint8_t pattern = 0xA5) {
    return ByteBuffer(size, pattern);
}

} // anonymous namespace

TEST_CASE("JobEngineTest", "[job]") {

    // JobTypes tests
    SECTION("testJobTypeEnum") {
    }
    SECTION("testJobStateEnum") {
    }
    SECTION("testJobPriorityEnum") {
    }
    SECTION("testJobStatisticsDefaults") {
    }
    SECTION("testJobResultDefaults") {
    }
    SECTION("testJobConfigDefaults") {
    }
    SECTION("testJobContextDefaults") {

    // RecoveryPolicy tests
    }
    SECTION("testDefaultRecoveryRetry") {
    }
    SECTION("testDefaultRecoveryAbortAfterRetries") {
    }
    SECTION("testDefaultRecoveryCancelled") {
    }
    SECTION("testDefaultRecoveryRollback") {
    }
    SECTION("testRetryForeverPolicy") {
    }
    SECTION("testAbortOnFailurePolicy") {
    }
    SECTION("testRollbackOnFailurePolicy") {

    // IJob interface tests
    }
    SECTION("testBaseJobDefaults") {
    }
    SECTION("testBaseJobCancel") {
    }
    SECTION("testBaseJobCanRollback") {

    // FlashJob tests
    }
    SECTION("testFlashJobPrepare") {
    }
    SECTION("testFlashJobPrepareNoDevice") {
    }
    SECTION("testFlashJobExecute") {
    }
    SECTION("testFlashJobExecuteCancelled") {
    }
    SECTION("testFlashJobRollback") {

    // BackupJob tests
    }
    SECTION("testBackupJobPrepare") {
    }
    SECTION("testBackupJobExecute") {
    }
    SECTION("testBackupJobRollback") {

    // RestoreJob tests
    }
    SECTION("testRestoreJobPrepare") {
    }
    SECTION("testRestoreJobExecute") {
    }
    SECTION("testRestoreJobRollback") {
    }
    SECTION("testRestoreJobEmptyDataFails") {

    // ReadJob tests
    }
    SECTION("testReadJobPrepare") {
    }
    SECTION("testReadJobExecute") {
    }
    SECTION("testReadJobRollback") {

    // EraseJob tests
    }
    SECTION("testEraseJobPrepare") {
    }
    SECTION("testEraseJobExecute") {
    }
    SECTION("testEraseJobRollback") {
    }
    SECTION("testEraseJobRollbackWithoutBackup") {

    // VerifyJob tests
    }
    SECTION("testVerifyJobPrepare") {
    }
    SECTION("testVerifyJobExecuteMatch") {
    }
    SECTION("testVerifyJobExecuteMismatch") {
    }
    SECTION("testVerifyJobRollback") {

    // GPTUpdateJob tests
    }
    SECTION("testGPTUpdateJobPrepare") {
    }
    SECTION("testGPTUpdateJobExecute") {
    }
    SECTION("testGPTUpdateJobRollback") {

    // ProgrammerUploadJob tests
    }
    SECTION("testProgrammerUploadPrepare") {
    }
    SECTION("testProgrammerUploadExecute") {
    }
    SECTION("testProgrammerUploadRollback") {

    // JobHistory tests
    }
    SECTION("testJobHistoryAddEntry") {
    }
    SECTION("testJobHistoryClear") {
    }
    SECTION("testJobHistoryRecent") {
    }
    SECTION("testJobHistoryCompleted") {
    }
    SECTION("testJobHistoryFailed") {
    }
    SECTION("testJobHistoryFilterByType") {
    }
    SECTION("testJobHistoryFilterBySuccess") {
    }
    SECTION("testJobHistoryFilterMaxResults") {
    }
    SECTION("testJobHistoryMaxEntries") {

    // ProgressAggregator tests
    }
    SECTION("testProgressAggregatorRegister") {
    }
    SECTION("testProgressAggregatorUpdate") {
    }
    SECTION("testProgressAggregatorComplete") {
    }
    SECTION("testProgressAggregatorFailed") {
    }
    SECTION("testProgressAggregatorOverallPercent") {
    }
    SECTION("testProgressAggregatorReset") {
    }
    SECTION("testProgressAggregatorCallback") {

    // JobPipeline tests
    }
    SECTION("testPipelineEmpty") {
    }
    SECTION("testPipelineSingleJob") {
    }
    SECTION("testPipelineMultipleJobs") {
    }
    SECTION("testPipelineCancel") {
    }
    SECTION("testPipelinePauseResume") {
    }
    SECTION("testPipelineClearJobs") {
    }
    SECTION("testPipelineRemoveJob") {
    }
    SECTION("testPipelineJobNotFound") {
    }
    SECTION("testPipelineProgressCallback") {
    }
    SECTION("testPipelineJobCallback") {
    }
    SECTION("testPipelineRollbackChain") {
    }
    SECTION("testPipelineHistoryPopulated") {

    // VirtualJobDevice tests
    }
    SECTION("testVirtualDeviceOpenClose") {
    }
    SECTION("testVirtualDeviceFailOpen") {
    }
    SECTION("testVirtualDeviceFailRead") {
    }
    SECTION("testVirtualDeviceFailWrite") {
    }
    SECTION("testVirtualDeviceFailErase") {
    }
    SECTION("testVirtualDeviceTimeoutRead") {
    }
    SECTION("testVirtualDeviceDisconnectOnWrite") {
    }
    SECTION("testVirtualDevicePartitionReadWrite") {
    }
    SECTION("testVirtualDevicePartitionNotFound") {
    }
    SECTION("testVirtualDeviceStorageReadWrite") {
    }
    SECTION("testVirtualDeviceReset") {
    }
    SECTION("testVirtualDeviceUploadLoader") {
    }
    SECTION("testVirtualDeviceGetPartitions") {
    }
    SECTION("testVirtualDeviceBadGPT") {
    }
    SECTION("testVirtualDeviceCorruptData") {
    }
    SECTION("testVirtualDeviceRandomFailures") {
    }
    SECTION("testVirtualDeviceCounters") {

    // JobScheduler tests
    }
    SECTION("testSchedulerEnqueue") {
    }
    SECTION("testSchedulerStartStop") {
    }
    SECTION("testSchedulerPauseResume") {
    }
    SECTION("testSchedulerStats") {

    // Integration tests
    }
    SECTION("testFlashThenVerify") {
    }
    SECTION("testBackupThenRestore") {
    }
    SECTION("testEraseThenRollback") {
    }
    SECTION("testPipelineWithMixedJobs") {
    }
}

// ============================================================
// JobTypes Tests
// ============================================================

void testJobTypeEnum() {
    REQUIRE(static_cast<int>(JobType::Flash) == 0);
    REQUIRE(static_cast<int>(JobType::Backup) == 1);
    REQUIRE(static_cast<int>(JobType::Restore) == 2);
    REQUIRE(static_cast<int>(JobType::Read) == 3);
    REQUIRE(static_cast<int>(JobType::Erase) == 4);
    REQUIRE(static_cast<int>(JobType::Verify) == 5);
    REQUIRE(static_cast<int>(JobType::ProgrammerUpload) == 6);
    REQUIRE(static_cast<int>(JobType::GPTUpdate) == 7);
    REQUIRE(static_cast<int>(JobType::Custom) == 8);
}

void testJobStateEnum() {
    REQUIRE(static_cast<int>(JobState::Pending) == 0);
    REQUIRE(static_cast<int>(JobState::Preparing) == 1);
    REQUIRE(static_cast<int>(JobState::Running) == 2);
    REQUIRE(static_cast<int>(JobState::Paused) == 3);
    REQUIRE(static_cast<int>(JobState::Completed) == 4);
    REQUIRE(static_cast<int>(JobState::Failed) == 5);
    REQUIRE(static_cast<int>(JobState::Cancelled) == 6);
    REQUIRE(static_cast<int>(JobState::RollingBack) == 7);
    REQUIRE(static_cast<int>(JobState::RolledBack) == 8);
}

void testJobPriorityEnum() {
    REQUIRE(static_cast<int>(JobPriority::Low) == 0);
    REQUIRE(static_cast<int>(JobPriority::Normal) == 1);
    REQUIRE(static_cast<int>(JobPriority::High) == 2);
    REQUIRE(static_cast<int>(JobPriority::Critical) == 3);
}

void testJobStatisticsDefaults() {
    JobStatistics stats;
    REQUIRE(stats.processedBytes == uint64_t(0));
    REQUIRE(stats.averageSpeedBps == 0.0);
    REQUIRE(stats.remainingBytes == uint64_t(0));
    REQUIRE(stats.retryCount == uint32_t(0));
    REQUIRE(stats.rollbackCount == uint32_t(0));
    REQUIRE(stats.failureCount == uint32_t(0));
}

void testJobResultDefaults() {
    JobResult result;
    REQUIRE(result.success == false);
    REQUIRE(static_cast<int>(result.error) == static_cast<int>(ErrorCode::Success));
    REQUIRE(result.errorMessage.empty());
}

void testJobConfigDefaults() {
    JobConfig config;
    REQUIRE(static_cast<int>(config.priority) == static_cast<int>(JobPriority::Normal));
    REQUIRE(config.maxRetries == 3);
    REQUIRE(config.maxRollbacks == 1);
    REQUIRE(config.enableRollback);
    REQUIRE(config.enableProgressCallback);
    REQUIRE(config.timeout.count() == int64_t(0));
    REQUIRE(config.data.empty());
    REQUIRE(config.partitionName.empty());
}

void testJobContextDefaults() {
    JobContext ctx;
    REQUIRE(ctx.flashDevice == nullptr);
    REQUIRE(ctx.partitionManager == nullptr);
    REQUIRE(ctx.session == nullptr);
    REQUIRE(ctx.loaderFramework == nullptr);
    REQUIRE(ctx.pipeline == nullptr);
    REQUIRE(ctx.logger == nullptr);
    REQUIRE(ctx.progressCallback == nullptr);
    REQUIRE(ctx.cancelled == nullptr);
}

// ============================================================
// RecoveryPolicy Tests
// ============================================================

void testDefaultRecoveryRetry() {
    DefaultRecoveryPolicy policy(3, 1, true);
    auto action = policy.evaluate("job1", ErrorCode::TransportError, 0);
    REQUIRE(static_cast<int>(action) == static_cast<int>(RecoveryAction::Retry));
}

void testDefaultRecoveryAbortAfterRetries() {
    DefaultRecoveryPolicy policy(2, 0, false);
    auto action = policy.evaluate("job1", ErrorCode::TransportError, 2);
    REQUIRE(static_cast<int>(action) == static_cast<int>(RecoveryAction::Abort));
}

void testDefaultRecoveryCancelled() {
    DefaultRecoveryPolicy policy(100, 5, true);
    auto action = policy.evaluate("job1", ErrorCode::Cancelled, 0);
    REQUIRE(static_cast<int>(action) == static_cast<int>(RecoveryAction::Abort));
}

void testDefaultRecoveryRollback() {
    DefaultRecoveryPolicy policy(1, 1, true);
    auto action = policy.evaluate("job1", ErrorCode::TransportError, 1);
    REQUIRE(static_cast<int>(action) == static_cast<int>(RecoveryAction::Rollback));
}

void testRetryForeverPolicy() {
    RetryForeverPolicy policy;
    auto a1 = policy.evaluate("j", ErrorCode::TransportError, 0);
    auto a2 = policy.evaluate("j", ErrorCode::TransportError, 1000);
    auto a3 = policy.evaluate("j", ErrorCode::TransportError, 100000);
    REQUIRE(static_cast<int>(a1) == static_cast<int>(RecoveryAction::Retry));
    REQUIRE(static_cast<int>(a2) == static_cast<int>(RecoveryAction::Retry));
    REQUIRE(static_cast<int>(a3) == static_cast<int>(RecoveryAction::Retry));
    REQUIRE(!policy.canRollback());
}

void testAbortOnFailurePolicy() {
    AbortOnFailurePolicy policy;
    auto a1 = policy.evaluate("j", ErrorCode::TransportError, 0);
    REQUIRE(static_cast<int>(a1) == static_cast<int>(RecoveryAction::Abort));
    REQUIRE(policy.maxRetries() == 0);
    REQUIRE(!policy.canRollback());
}

void testRollbackOnFailurePolicy() {
    RollbackOnFailurePolicy policy(2);
    auto a1 = policy.evaluate("j", ErrorCode::TransportError, 0);
    REQUIRE(static_cast<int>(a1) == static_cast<int>(RecoveryAction::Rollback));
    REQUIRE(policy.maxRetries() == 0);
    REQUIRE(policy.canRollback());
}

// ============================================================
// IJob / BaseJob Tests
// ============================================================

void testBaseJobDefaults() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    auto job = std::make_unique<FlashJob>("test1", "boot", makeData(64));
    REQUIRE(job->id() == "test1");
    REQUIRE(static_cast<int>(job->type()) == static_cast<int>(JobType::Flash));
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Pending));
    REQUIRE(job->recoveryPolicy() != nullptr);
    REQUIRE(job->result().success == false);

    auto prepResult = job->prepare(ctx);
    REQUIRE(prepResult.isOk());

    auto execResult = job->execute(ctx);
    REQUIRE(execResult.isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
    REQUIRE(job->result().success);
}

void testBaseJobCancel() {
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<FlashJob>("cancel_test", "boot", makeData(64));
    job->cancel();
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Cancelled));

    auto execResult = job->execute(ctx);
    REQUIRE(execResult.isError());
    REQUIRE(static_cast<int>(execResult.error()) == static_cast<int>(ErrorCode::Cancelled));
}

void testBaseJobCanRollback() {
    auto job = std::make_unique<FlashJob>("rb_test", "boot", makeData(64));
    REQUIRE(job->canRollback());

    job->setRecoveryPolicy(std::make_unique<AbortOnFailurePolicy>());
    REQUIRE(!job->canRollback());
}

// ============================================================
// FlashJob Tests
// ============================================================

void testFlashJobPrepare() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 1024 * 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<FlashJob>("f1", "boot", makeData(256));
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testFlashJobPrepareNoDevice() {
    JobContext ctx;
    auto job = std::make_unique<FlashJob>("f2", "boot", makeData(256));
    auto result = job->prepare(ctx);
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::JobDeviceNotReady));
}

void testFlashJobExecute() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 1024 * 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<FlashJob>("f3", "boot", makeData(256));
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
    REQUIRE(job->result().success);
}

void testFlashJobExecuteCancelled() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 1024 * 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<FlashJob>("f4", "boot", makeData(256));
    REQUIRE(job->prepare(ctx).isOk());
    job->cancel();
    auto result = job->execute(ctx);
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::Cancelled));
}

void testFlashJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 1024 * 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<FlashJob>("f5", "boot", makeData(256));
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// BackupJob Tests
// ============================================================

void testBackupJobPrepare() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 1024 * 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<BackupJob>("b1", "data");
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testBackupJobExecute() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);
    device.writeToStorage(0, makeData(256));

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<BackupJob>("b2", "data");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
    REQUIRE(!job->backupData().empty());
}

void testBackupJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<BackupJob>("b3", "data");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// RestoreJob Tests
// ============================================================

void testRestoreJobPrepare() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<RestoreJob>("r1", "data", makeData(128));
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testRestoreJobExecute() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto data = makeData(128, 0xBB);
    auto job = std::make_unique<RestoreJob>("r2", "data", data);
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
}

void testRestoreJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<RestoreJob>("r3", "data", makeData(64));
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

void testRestoreJobEmptyDataFails() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    auto job = std::make_unique<RestoreJob>("r4", "data", ByteBuffer{});
    auto result = job->prepare(ctx);
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::InvalidArgument));
}

// ============================================================
// ReadJob Tests
// ============================================================

void testReadJobPrepare() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("config", 0, 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<ReadJob>("rd1", "config");
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testReadJobExecute() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("config", 0, 4096);
    auto expected = makeData(128, 0xCC);
    device.writeToStorage(0, expected);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<ReadJob>("rd2", "config");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
    REQUIRE(!job->readData().empty());
}

void testReadJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("config", 0, 1024);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<ReadJob>("rd3", "config");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// EraseJob Tests
// ============================================================

void testEraseJobPrepare() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("cache", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<EraseJob>("e1", "cache");
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testEraseJobExecute() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("cache", 0, 4096);
    device.writeToStorage(0, makeData(256, 0xFF));

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<EraseJob>("e2", "cache");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
}

void testEraseJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("cache", 0, 4096);
    auto original = makeData(128, 0xAA);
    device.writeToStorage(0, original);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<EraseJob>("e3", "cache");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());

    auto readBack = device.readFromStorage(0, 128);
    REQUIRE(readBack == original);
}

void testEraseJobRollbackWithoutBackup() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("empty", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<EraseJob>("e4", "empty");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// VerifyJob Tests
// ============================================================

void testVerifyJobPrepare() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("system", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<VerifyJob>("v1", "system", makeData(64));
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testVerifyJobExecuteMatch() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("system", 0, 4096);
    auto data = makeData(4096, 0xDD);
    device.writeToStorage(0, data);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<VerifyJob>("v2", "system", data);
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
    REQUIRE(job->result().success);
}

void testVerifyJobExecuteMismatch() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("system", 0, 4096);
    device.writeToStorage(0, makeData(256, 0xDD));

    JobContext ctx;
    ctx.flashDevice = &device;

    auto expected = makeData(256, 0xEE);
    auto job = std::make_unique<VerifyJob>("v3", "system", expected);
    REQUIRE(job->prepare(ctx).isOk());
    auto result = job->execute(ctx);
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::JobDataMismatch));
}

void testVerifyJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("system", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<VerifyJob>("v4", "system", makeData(64));
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// GPTUpdateJob Tests
// ============================================================

void testGPTUpdateJobPrepare() {
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<GPTUpdateJob>("g1");
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testGPTUpdateJobExecute() {
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<GPTUpdateJob>("g2");
    REQUIRE(job->prepare(ctx).isOk());
    auto result = job->execute(ctx);
    REQUIRE(result.isOk());
}

void testGPTUpdateJobRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("gpt_primary", 0, 17408);

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<GPTUpdateJob>("g3");
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// ProgrammerUploadJob Tests
// ============================================================

void testProgrammerUploadPrepare() {
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<ProgrammerUploadJob>("p1", makeData(1024));
    auto result = job->prepare(ctx);
    REQUIRE(result.isOk());
}

void testProgrammerUploadExecute() {
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<ProgrammerUploadJob>("p2", makeData(1024));
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    REQUIRE(static_cast<int>(job->state()) == static_cast<int>(JobState::Completed));
    REQUIRE(job->result().success);
}

void testProgrammerUploadRollback() {
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;

    auto job = std::make_unique<ProgrammerUploadJob>("p3", makeData(1024));
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());
    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());
}

// ============================================================
// JobHistory Tests
// ============================================================

void testJobHistoryAddEntry() {
    JobHistory history;
    JobHistoryEntry entry;
    entry.jobId = "test1";
    entry.type = JobType::Flash;
    entry.success = true;

    history.addEntry(entry);
    REQUIRE(history.totalCount() == std::size_t(1));
}

void testJobHistoryClear() {
    JobHistory history;
    history.addEntry(JobHistoryEntry{});
    history.addEntry(JobHistoryEntry{});
    REQUIRE(history.totalCount() == std::size_t(2));

    history.clear();
    REQUIRE(history.totalCount() == std::size_t(0));
}

void testJobHistoryRecent() {
    JobHistory history;
    for (int i = 0; i < 5; i++) {
        JobHistoryEntry e;
        e.jobId = "job" + std::to_string(i);
        history.addEntry(e);
    }

    auto recent = history.recent(3);
    REQUIRE(recent.size() == std::size_t(3));
}

void testJobHistoryCompleted() {
    JobHistory history;
    JobHistoryEntry e1; e1.jobId = "ok1"; e1.success = true;
    JobHistoryEntry e2; e2.jobId = "fail1"; e2.success = false;
    JobHistoryEntry e3; e3.jobId = "ok2"; e3.success = true;
    history.addEntry(e1);
    history.addEntry(e2);
    history.addEntry(e3);

    auto completed = history.completed();
    REQUIRE(completed.size() == std::size_t(2));
    REQUIRE(history.successCount() == std::size_t(2));
}

void testJobHistoryFailed() {
    JobHistory history;
    JobHistoryEntry e1; e1.jobId = "ok1"; e1.success = true;
    JobHistoryEntry e2; e2.jobId = "fail1"; e2.success = false;
    JobHistoryEntry e3; e3.jobId = "fail2"; e3.success = false;
    history.addEntry(e1);
    history.addEntry(e2);
    history.addEntry(e3);

    auto failed = history.failed();
    REQUIRE(failed.size() == std::size_t(2));
    REQUIRE(history.failureCount() == std::size_t(2));
}

void testJobHistoryFilterByType() {
    JobHistory history;
    JobHistoryEntry e1; e1.jobId = "f1"; e1.type = JobType::Flash;
    JobHistoryEntry e2; e2.jobId = "b1"; e2.type = JobType::Backup;
    JobHistoryEntry e3; e3.jobId = "f2"; e3.type = JobType::Flash;
    history.addEntry(e1);
    history.addEntry(e2);
    history.addEntry(e3);

    HistoryFilter filter;
    filter.types = {JobType::Flash};
    auto result = history.find(filter);
    REQUIRE(result.size() == std::size_t(2));
}

void testJobHistoryFilterBySuccess() {
    JobHistory history;
    JobHistoryEntry e1; e1.jobId = "ok1"; e1.success = true;
    JobHistoryEntry e2; e2.jobId = "fail1"; e2.success = false;
    history.addEntry(e1);
    history.addEntry(e2);

    HistoryFilter filter;
    filter.successFilter = {true};
    auto result = history.find(filter);
    REQUIRE(result.size() == std::size_t(1));
    REQUIRE(result[0].jobId == "ok1");
}

void testJobHistoryFilterMaxResults() {
    JobHistory history;
    for (int i = 0; i < 10; i++) {
        JobHistoryEntry e; e.jobId = "j" + std::to_string(i);
        history.addEntry(e);
    }

    HistoryFilter filter;
    filter.maxResults = 3;
    auto result = history.find(filter);
    REQUIRE(result.size() <= std::size_t(3));
}

void testJobHistoryMaxEntries() {
    JobHistory history;
    history.setMaxEntries(5);

    for (int i = 0; i < 10; i++) {
        JobHistoryEntry e; e.jobId = "j" + std::to_string(i);
        history.addEntry(e);
    }

    REQUIRE(history.totalCount() <= std::size_t(5));
}

// ============================================================
// ProgressAggregator Tests
// ============================================================

void testProgressAggregatorRegister() {
    ProgressAggregator agg;
    agg.registerJob("j1", JobType::Flash, 1000);
    agg.registerJob("j2", JobType::Backup, 2000);

    auto ap = agg.current();
    REQUIRE(ap.totalJobs == 0);
}

void testProgressAggregatorUpdate() {
    ProgressAggregator agg;
    agg.registerJob("j1", JobType::Flash, 1000);

    ProgressInfo pi;
    pi.totalBytes = 1000;
    pi.transferredBytes = 500;
    pi.percentage = 50.0;
    agg.updateJobProgress("j1", pi);

    auto ap = agg.current();
    REQUIRE(ap.totalBytes == uint64_t(1000));
    REQUIRE(ap.transferredBytes == uint64_t(500));
}

void testProgressAggregatorComplete() {
    ProgressAggregator agg;
    agg.registerJob("j1", JobType::Flash, 100);
    agg.markJobCompleted("j1");

    auto ap = agg.current();
    REQUIRE(ap.completedJobs == 1);
}

void testProgressAggregatorFailed() {
    ProgressAggregator agg;
    agg.registerJob("j1", JobType::Flash, 100);
    agg.markJobFailed("j1");

    auto ap = agg.current();
    REQUIRE(ap.failedJobs == 1);
}

void testProgressAggregatorOverallPercent() {
    ProgressAggregator agg;
    agg.registerJob("j1", JobType::Flash, 200);
    agg.setTotalBytes(200);

    ProgressInfo pi;
    pi.totalBytes = 200;
    pi.transferredBytes = 100;
    pi.percentage = 50.0;
    agg.updateJobProgress("j1", pi);

    auto ap = agg.current();
    REQUIRE(ap.overallPercentage > 0.0);
}

void testProgressAggregatorReset() {
    ProgressAggregator agg;
    agg.registerJob("j1", JobType::Flash, 100);
    agg.markJobCompleted("j1");
    agg.reset();

    auto ap = agg.current();
    REQUIRE(ap.completedJobs == 0);
    REQUIRE(ap.failedJobs == 0);
}

void testProgressAggregatorCallback() {
    ProgressAggregator agg;
    int callbackCount = 0;

    agg.setCallback([&](const AggregatedProgress&) {
        callbackCount++;
    });

    agg.registerJob("j1", JobType::Flash, 100);
    ProgressInfo pi;
    pi.transferredBytes = 50;
    pi.totalBytes = 100;
    agg.updateJobProgress("j1", pi);

    REQUIRE(callbackCount > 0);
}

// ============================================================
// JobPipeline Tests
// ============================================================

void testPipelineEmpty() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.jobCount() == std::size_t(0));
}

void testPipelineSingleJob() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));

    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.completedCount() == std::size_t(1));
}

void testPipelineMultipleJobs() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);
    device.addPartition("system", 4096, 8192);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    pipeline.addJob(std::make_unique<FlashJob>("f2", "system", makeData(256)));

    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.completedCount() == std::size_t(2));
}

void testPipelineCancel() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));

    pipeline.cancel();
    REQUIRE(pipeline.isCancelled());
    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk() == pipeline.isRunning());
}

void testPipelinePauseResume() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);
    device.addPartition("system", 4096, 8192);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    pipeline.addJob(std::make_unique<FlashJob>("f2", "system", makeData(256)));

    pipeline.pause();
    REQUIRE(pipeline.isPaused());
    pipeline.resume();
    REQUIRE(!pipeline.isPaused());

    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.completedCount() == std::size_t(2));
}

void testPipelineClearJobs() {
    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(64)));
    pipeline.addJob(std::make_unique<FlashJob>("f2", "system", makeData(128)));
    REQUIRE(pipeline.jobCount() == std::size_t(2));

    pipeline.clearJobs();
    REQUIRE(pipeline.jobCount() == std::size_t(0));
}

void testPipelineRemoveJob() {
    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(64)));
    pipeline.addJob(std::make_unique<FlashJob>("f2", "system", makeData(128)));
    REQUIRE(pipeline.jobCount() == std::size_t(2));

    pipeline.removeJob("f1");
    REQUIRE(pipeline.jobCount() == std::size_t(1));

    auto* job = pipeline.findJob("f2");
    REQUIRE(job != nullptr);
    REQUIRE(pipeline.findJob("f1") == nullptr);
}

void testPipelineJobNotFound() {
    JobPipeline pipeline;
    REQUIRE(pipeline.findJob("nonexistent") == nullptr);
}

void testPipelineProgressCallback() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    int progressCount = 0;
    JobPipeline pipeline;
    pipeline.setProgressCallback([&](const AggregatedProgress&) {
        progressCount++;
    });

    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    pipeline.run(ctx);

    REQUIRE(progressCount >= 0);
}

void testPipelineJobCallback() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    int cbCount = 0;
    JobPipeline pipeline;
    pipeline.setJobCallback([&](const std::string&, JobState, bool) {
        cbCount++;
    });

    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    pipeline.run(ctx);

    REQUIRE(cbCount == 1);
}

void testPipelineRollbackChain() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("good", 0, 4096);
    device.addPartition("bad", 4096, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    device.setFailureRules([]{
        FailureRule r;
        r.rollbackSuccess = true;
        return r;
    }());

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "good", makeData(128)));
    pipeline.addJob(std::make_unique<FlashJob>("f2", "bad", makeData(256)));

    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
}

void testPipelineHistoryPopulated() {
    NullTestLogger logger;
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    pipeline.run(ctx);

    REQUIRE(pipeline.history().totalCount() == std::size_t(1));
}

// ============================================================
// VirtualJobDevice Tests
// ============================================================

void testVirtualDeviceOpenClose() {
    VirtualJobDevice device;
    REQUIRE(device.open().isOk());
    REQUIRE(device.isOpen());
    REQUIRE(device.openCount() == 1);

    device.close();
    REQUIRE(!device.isOpen());
    REQUIRE(device.closeCount() == 1);
}

void testVirtualDeviceFailOpen() {
    VirtualJobDevice device;
    device.failureRules().failOpen = true;
    auto result = device.open();
    REQUIRE(result.isError());
}

void testVirtualDeviceFailRead() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().failRead = true;

    auto result = device.readMemory(0, 64);
    REQUIRE(result.isError());
}

void testVirtualDeviceFailWrite() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().failWrite = true;

    auto result = device.writeMemory(0, makeData(64));
    REQUIRE(result.isError());
}

void testVirtualDeviceFailErase() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().failErase = true;

    auto result = device.eraseMemory(0, 64);
    REQUIRE(result.isError());
}

void testVirtualDeviceTimeoutRead() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().timeoutOnRead = true;
    device.setTimeouts(std::chrono::milliseconds(5), {}, {});

    auto result = device.readMemory(0, 64);
    REQUIRE(result.isOk());
}

void testVirtualDeviceDisconnectOnWrite() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().disconnectOnWrite = true;

    auto result = device.writeMemory(0, makeData(64));
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::DeviceDisconnected));
}

void testVirtualDevicePartitionReadWrite() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("test", 0, 4096);

    auto data = makeData(4096, 0xBB);
    auto writeResult = device.writePartition("test", data);
    REQUIRE(writeResult.isOk());

    auto readResult = device.readPartition("test");
    REQUIRE(readResult.isOk());
    REQUIRE(readResult.value().size() == size_t(4096));
    REQUIRE(readResult.value() == data);
}

void testVirtualDevicePartitionNotFound() {
    VirtualJobDevice device;
    device.open();

    auto result = device.readPartition("nonexistent");
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::PartitionNotFound));
}

void testVirtualDeviceStorageReadWrite() {
    VirtualJobDevice device;
    device.open();
    device.setStorageSize(8192);

    auto data = makeData(512, 0xAA);
    REQUIRE(device.writeMemory(0, data).isOk());

    auto readBack = device.readMemory(0, 512);
    REQUIRE(readBack.isOk());
    REQUIRE(readBack.value() == data);
}

void testVirtualDeviceReset() {
    VirtualJobDevice device;
    device.open();

    auto result = device.reset();
    REQUIRE(result.isOk());
}

void testVirtualDeviceUploadLoader() {
    VirtualJobDevice device;
    device.open();

    auto result = device.uploadLoader(makeData(1024));
    REQUIRE(result.isOk());
}

void testVirtualDeviceGetPartitions() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("p1", 0, 1024);
    device.addPartition("p2", 1024, 2048);

    auto result = device.getPartitions();
    REQUIRE(result.isOk());
    REQUIRE(result.value().entryCount == uint32_t(2));
}

void testVirtualDeviceBadGPT() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().badGPT = true;

    auto result = device.getPartitions();
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(result.error()) == static_cast<int>(ErrorCode::GPTCorrupted));
}

void testVirtualDeviceCorruptData() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().corruptData = true;
    device.addPartition("p1", 0, 4096);

    auto original = makeData(64, 0xAA);
    device.writePartition("p1", original);

    auto readBack = device.readPartition("p1");
    REQUIRE(readBack.isOk());
    REQUIRE(readBack.value() != original);
}

void testVirtualDeviceRandomFailures() {
    VirtualJobDevice device;
    device.open();
    device.failureRules().randomFailureRate = 1.0;
    device.failureRules().seed = 12345;

    bool hadFailure = false;
    for (int i = 0; i < 20; i++) {
        auto result = device.readMemory(0, 64);
        if (result.isError()) hadFailure = true;
    }
    REQUIRE(hadFailure);
}

void testVirtualDeviceCounters() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("p1", 0, 4096);

    device.readMemory(0, 64);
    device.writeMemory(0, makeData(64));
    device.eraseMemory(0, 64);
    device.readPartition("p1");
    device.writePartition("p1", makeData(64));
    device.erasePartition("p1");

    REQUIRE(device.readCount() == 2);
    REQUIRE(device.writeCount() == 2);
    REQUIRE(device.eraseCount() == 2);

    device.resetCounters();
    REQUIRE(device.readCount() == 0);
    REQUIRE(device.writeCount() == 0);
    REQUIRE(device.eraseCount() == 0);
}

// ============================================================
// JobScheduler Tests
// ============================================================

void testSchedulerEnqueue() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobScheduler scheduler(ctx);
    scheduler.enqueue(std::make_unique<FlashJob>("f1", "boot", makeData(128)));

    auto stats = scheduler.stats();
    REQUIRE(stats.totalScheduled == std::size_t(1));
}

void testSchedulerStartStop() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobScheduler scheduler(ctx);
    scheduler.enqueue(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    scheduler.start();

    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (scheduler.stats().totalCompleted < 1 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    scheduler.stop();

    auto stats = scheduler.stats();
    REQUIRE(stats.totalCompleted == std::size_t(1));
}

void testSchedulerPauseResume() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobScheduler scheduler(ctx);
    scheduler.enqueue(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    scheduler.start();

    scheduler.pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    scheduler.resume();

    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (scheduler.stats().totalCompleted < 1 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    scheduler.stop();
}

void testSchedulerStats() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobScheduler scheduler(ctx);
    scheduler.enqueue(std::make_unique<FlashJob>("f1", "boot", makeData(128)));
    scheduler.enqueue(std::make_unique<FlashJob>("f2", "boot", makeData(256)));

    scheduler.start();
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (scheduler.stats().totalCompleted < 1 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    scheduler.stop();

    auto stats = scheduler.stats();
    REQUIRE(stats.totalCompleted >= std::size_t(1));
}

// ============================================================
// Integration Tests
// ============================================================

void testFlashThenVerify() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    auto firmware = makeData(4096, 0x77);

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("flash", "boot", firmware));
    pipeline.addJob(std::make_unique<VerifyJob>("verify", "boot", firmware));

    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.completedCount() == std::size_t(2));
    REQUIRE(pipeline.failedCount() == std::size_t(0));
}

void testBackupThenRestore() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    auto original = makeData(128, 0x99);
    device.writeToStorage(0, original);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<BackupJob>("backup", "data"));
    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());

    device.eraseMemory(0, 4096);

    // Now flash some different data
    auto newData = makeData(128, 0x88);
    device.writeToStorage(0, newData);

    // Now restore back to original
    pipeline.clearJobs();
    pipeline.addJob(std::make_unique<RestoreJob>("restore", "data", original));
    result = pipeline.run(ctx);
    REQUIRE(result.isOk());

    // Verify original data is back
    auto readBack = device.readFromStorage(0, 128);
    REQUIRE(readBack == original);
}

void testEraseThenRollback() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("data", 0, 4096);

    auto original = makeData(256, 0x55);
    device.writeToStorage(0, original);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    auto job = std::make_unique<EraseJob>("erase", "data");
    REQUIRE(job->prepare(ctx).isOk());
    REQUIRE(job->execute(ctx).isOk());

    auto erased = device.readFromStorage(0, 256);
    REQUIRE(erased != original);

    auto rbResult = job->rollback(ctx);
    REQUIRE(rbResult.isOk());

    auto restored = device.readFromStorage(0, 256);
    REQUIRE(restored == original);
}

void testPipelineWithMixedJobs() {
    VirtualJobDevice device;
    device.open();
    device.addPartition("boot", 0, 4096);
    device.addPartition("system", 4096, 8192);
    device.addPartition("cache", 12288, 4096);

    NullTestLogger logger;
    JobContext ctx;
    ctx.flashDevice = &device;
    ctx.logger = &logger;

    JobPipeline pipeline;
    pipeline.addJob(std::make_unique<FlashJob>("flash_boot", "boot", makeData(4096, 0x11)));
    pipeline.addJob(std::make_unique<FlashJob>("flash_system", "system", makeData(8192, 0x22)));
    pipeline.addJob(std::make_unique<VerifyJob>("verify_boot", "boot", makeData(4096, 0x11)));
    pipeline.addJob(std::make_unique<VerifyJob>("verify_system", "system", makeData(8192, 0x22)));

    auto result = pipeline.run(ctx);
    REQUIRE(result.isOk());
    REQUIRE(pipeline.completedCount() == std::size_t(4));
    REQUIRE(pipeline.failedCount() == std::size_t(0));
}

