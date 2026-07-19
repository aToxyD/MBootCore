# Workflow Execution

Demonstrates:
- Building a custom workflow using `WorkflowBuilder`
- Adding standard steps (connect, detect, negotiate, flash, verify, reboot, disconnect)
- Adding custom `IWorkflowStep` implementations
- Configuring workflow options (retry, rollback, timeout)
- Executing workflow with `prepare()` / `run()`
- Inspecting progress and statistics
- Performing workflow rollback

## Build

```bash
cd build
cmake --build . --target example_workflow_execution
```

## Run

```bash
./example_workflow_execution
```
