#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <vector>

namespace mbootcore {
namespace session {

enum class SessionState : uint32_t {
    Disconnected  = 0,
    Discovered    = 1,
    Negotiated    = 2,
    Connected     = 3,
    Initializing  = 4,
    Ready         = 5,
    Busy          = 6,
    Reading       = 7,
    Writing       = 8,
    Erasing       = 9,
    Resetting     = 10,
    Finished      = 11,
    Error         = 12,
    Cancelled     = 13,
    Connecting    = 14
};

enum class SessionEvent : uint32_t {
    Connect     = 0,
    Disconnect  = 1,
    Cancel      = 2,
    Retry       = 3,
    Reset       = 4,
    Timeout     = 5,
    Progress    = 6,
    Completed   = 7,
    Error       = 8,
    Reconnect   = 9
};

struct SessionStatistics {
    std::chrono::steady_clock::time_point connectionTime;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds elapsedTime{0};

    uint64_t bytesRead{0};
    uint64_t bytesWritten{0};
    uint64_t bytesErased{0};

    uint32_t readOps{0};
    uint32_t writeOps{0};
    uint32_t eraseOps{0};
    uint32_t retries{0};
    uint32_t failures{0};

    double averageReadBps{0.0};
    double maxReadBps{0.0};
    double averageWriteBps{0.0};
    double maxWriteBps{0.0};
};

struct SessionOperation {
    std::string name;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds duration{0};
    uint64_t bytesTransferred{0};
    bool success{true};
    std::string errorMessage;
};

struct SessionConfig {
    std::chrono::milliseconds connectTimeout{5000};
    std::chrono::milliseconds operationTimeout{30000};
    std::chrono::milliseconds progressInterval{250};

    int maxRetries{3};
    int maxRecoveryAttempts{2};

    bool enableLogging{true};
    bool enableHistory{true};
    bool enableStatistics{true};
    bool enableAutoRecovery{true};
    bool cancelOnError{false};

    std::string sessionName;
};

} // namespace session
} // namespace mbootcore
