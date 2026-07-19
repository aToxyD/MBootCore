#pragma once

#include <cstdint>

namespace mbootcore {
namespace runtime {

enum class LockMode : uint8_t {
    None,
    Exclusive,
};

enum class Operation : uint8_t {
    Discover,
    Probe,
    Connect,
    Disconnect,
    Reconnect,
    Flash,
    Read,
    Write,
    Erase,
    Verify,
    ReadPartition,
    WritePartition,
    ErasePartition,
    Backup,
    Restore,
    LoadFirmwarePackage,
    ExecuteWorkflow,
    RunJob,
    RunJobs,
    InstallPlugin,
    RemovePlugin,
    RegisterVendor,
    Initialize,
    Shutdown,
};

struct OperationOptions {
    LockMode lockMode = LockMode::None;
};

} // namespace runtime
} // namespace mbootcore
