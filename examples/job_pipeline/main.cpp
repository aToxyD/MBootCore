#include <mbootcore/MBootCore.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/job/JobPipeline.hpp>
#include <mbootcore/job/JobScheduler.hpp>
#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/job/JobTypes.hpp>
#include <mbootcore/job/ProgressAggregator.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>

using namespace mbootcore;

int main() {
    auto logger = std::make_shared<ConsoleLogger>();
    logger->info("example", "Job Pipeline Example\n");

    auto runtime = runtime::RuntimeFactory::createDefault();
    auto initResult = runtime.initialize();
    if (!initResult.isOk()) {
        std::cerr << "Runtime init failed: " << toString(initResult.error()) << std::endl;
        return EXIT_FAILURE;
    }

    // Create a JobContext
    job::JobContext jobCtx;
    jobCtx.logger = logger.get();

    runtime::RuntimeCallbacks cb;
    cb.onJobProgress =
        [&](const std::string& jobId, const ProgressInfo& info) {
            logger->info("jobs",
                         "[" + jobId + "] " +
                             std::to_string(static_cast<int>(info.percentage)) +
                             "% @ " + std::to_string(static_cast<int>(info.speedBps)) +
                             " B/s - " + info.currentOperation);
        };
    runtime.setCallbacks(cb);

    // Create sample data
    ByteBuffer sampleData(4096, 0xAB);
    ByteBuffer verifyData(4096, 0xAB);
    ByteBuffer restoreData(4096, 0xCD);

    // Build job pipeline
    job::JobPipeline pipeline;

    // Programmer upload job
    auto programmerJob = std::make_unique<job::ProgrammerUploadJob>(
        "prog-001", sampleData);
    pipeline.addJob(std::move(programmerJob));

    // Flash jobs
    auto flashBoot = std::make_unique<job::FlashJob>(
        "flash-boot", "boot", sampleData);
    pipeline.addJob(std::move(flashBoot));

    auto flashSystem = std::make_unique<job::FlashJob>(
        "flash-system", "system", sampleData);
    pipeline.addJob(std::move(flashSystem));

    // GPT update
    auto gptJob = std::make_unique<job::GPTUpdateJob>("gpt-update");
    pipeline.addJob(std::move(gptJob));

    // Verify
    auto verifyJob = std::make_unique<job::VerifyJob>(
        "verify-boot", "boot", verifyData);
    pipeline.addJob(std::move(verifyJob));

    // Backup
    auto backupJob = std::make_unique<job::BackupJob>("backup-data", "userdata");
    pipeline.addJob(std::move(backupJob));

    logger->info("example",
                 "Pipeline created with " + std::to_string(pipeline.jobCount()) + " jobs");

    // Set up progress callback
    pipeline.setProgressCallback([&](const job::AggregatedProgress& ap) {
        logger->info("pipeline",
                     "Progress: [" + std::to_string(ap.completedJobs) + "/" +
                         std::to_string(ap.totalJobs) + "] " +
                         std::to_string(static_cast<int>(ap.overallPercentage)) +
                         "% - " + ap.currentOperation +
                         " (" + std::to_string(ap.transferredBytes) + "/" +
                         std::to_string(ap.totalBytes) + " bytes)");
    });

    pipeline.setJobCallback([&](const std::string& jobId, job::JobState state, bool success) {
        logger->info("pipeline",
                     "Job '" + jobId + "' " +
                         (success ? "completed" : "failed") +
                         " (state=" + std::to_string(static_cast<int>(state)) + ")");
    });

    // Run pipeline
    logger->info("example", "\n--- Running job pipeline ---\n");
    auto runResult = pipeline.run(jobCtx);

    if (runResult.isOk()) {
        logger->info("example",
                     "\nPipeline completed: " +
                         std::to_string(pipeline.completedCount()) + " succeeded, " +
                         std::to_string(pipeline.failedCount()) + " failed");
    } else {
        logger->error("example",
                      "\nPipeline failed: " + std::string(toString(runResult.error())));
    }

    // Demonstrate JobScheduler
    logger->info("example", "\n--- Demonstrating JobScheduler ---\n");

    job::JobContext schedCtx;
    schedCtx.logger = logger.get();
    job::JobScheduler scheduler(schedCtx);

    scheduler.setPipelineCallback([&](const job::AggregatedProgress& ap) {
        logger->info("scheduler",
                     "Scheduled pipeline: " + std::to_string(static_cast<int>(ap.overallPercentage)) +
                         "% - " + ap.currentOperation);
    });

    // Enqueue jobs
    scheduler.enqueue(std::make_unique<job::EraseJob>("erase-cache", "cache"));
    scheduler.enqueue(std::make_unique<job::FlashJob>("flash-vendor", "vendor", sampleData));
    scheduler.enqueue(std::make_unique<job::RestoreJob>("restore-data", "userdata", restoreData));

    auto schedStats = scheduler.stats();
    logger->info("example",
                 "Scheduler queue: " + std::to_string(schedStats.queueSize) +
                     " jobs, " + std::to_string(schedStats.totalScheduled) + " total scheduled");

    // Start the scheduler
    scheduler.start();
    logger->info("example", "Scheduler started (will process asynchronously)");

    // Give scheduler time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    scheduler.stop();
    logger->info("example", "Scheduler stopped");

    auto finalStats = scheduler.stats();
    logger->info("example",
                 "Scheduler final state: " +
                     std::to_string(finalStats.totalCompleted) + " completed, " +
                     std::to_string(finalStats.totalFailed) + " failed, " +
                     std::to_string(finalStats.totalCancelled) + " cancelled");

    // Show history
    auto& history = scheduler.history();
    logger->info("example",
                 "Job history entries: " + std::to_string(history.totalCount()));

    runtime.shutdown();
    return EXIT_SUCCESS;
}
