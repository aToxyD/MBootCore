#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace mbootcore { namespace faults {

enum class FaultType : uint32_t {
    None=0, TransportFailure=1, MemoryFailure=2, Timeout=3,
    CRCError=4, GPTCorruption=5, DSPCorruption=6,
    PluginFailure=7, WorkflowFailure=8, LoaderFailure=9,
    USBDisconnect=10, SerialDisconnect=11, TCPDisconnect=12,
    PartialWrite=13, PartialRead=14, PowerInterruption=15
};

enum class FaultScope : uint32_t { Once=0, Random=1, Persistent=2, Timed=3 };
enum class FaultState : uint32_t { Inactive=0, Active=1, Triggered=2, Expired=3 };

struct FaultConfig {
    FaultType type{FaultType::None};
    FaultScope scope{FaultScope::Once};
    uint32_t probability{100};
    uint32_t maxTriggers{1};
    std::chrono::milliseconds duration{0};
    bool enabled{false};
};

struct FaultEvent {
    FaultType type;
    std::string description;
    std::chrono::steady_clock::time_point timestamp;
    bool handled{false};
};

class FaultInjector;
class FaultManager;

} }
