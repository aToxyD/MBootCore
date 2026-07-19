#pragma once
#include <cstdint>

namespace mbootcore { namespace api {

enum class ApiVersion : uint32_t {
    V1_0 = 0x01000000,
    Current = V1_0
};

enum class ApiCapability : uint32_t {
    None              = 0,
    Diagnostics       = 1 << 0,
    Performance       = 1 << 1,
    MemoryTracking    = 1 << 2,
    StressTesting     = 1 << 3,
    FaultInjection    = 1 << 4,
    RecoveryValidation = 1 << 5,
    Security          = 1 << 6,
    Configuration     = 1 << 7,
    Telemetry         = 1 << 8,
    StructuredLogging = 1 << 9,
    Benchmarking      = 1 << 10,
    Compatibility     = 1 << 11,
    FuzzTesting       = 1 << 12,
    LongRunning       = 1 << 13
};

} }
