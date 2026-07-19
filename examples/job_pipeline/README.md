# Job Pipeline

Demonstrates:
- Creating various job types via `GenericJobs` (FlashJob, BackupJob, VerifyJob, etc.)
- Building and executing a `JobPipeline`
- Setting up pipeline-level progress and job callbacks
- Using `JobScheduler` for async job queue management
- Enqueuing jobs with the scheduler
- Inspecting job history

## Build

```bash
cd build
cmake --build . --target example_job_pipeline
```

## Run

```bash
./example_job_pipeline
```
