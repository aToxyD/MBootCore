#pragma once

#include <mbootcore/domain/Error.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace mbootcore {
namespace transport {

enum class TransportType : uint32_t {
    Unknown = 0,
    Mock,
    Virtual,
    USB,
    Serial,
    TCP,
    UDP,
    Bluetooth,
    HID
};

enum class TransportState : uint32_t {
    Closed  = 0,
    Opening,
    Open,
    Closing,
    Error
};

enum class TransportCapability : uint32_t {
    None           = 0,
    Read           = 1 << 0,
    Write          = 1 << 1,
    AsyncRead      = 1 << 2,
    AsyncWrite     = 1 << 3,
    Hotplug        = 1 << 4,
    Streaming      = 1 << 5,
    Timeout        = 1 << 6,
    Cancellation   = 1 << 7,
    Reconnectable  = 1 << 8,
    Configurable   = 1 << 9,
    Observable     = 1 << 10,
    BulkTransfer   = 1 << 11,
    ControlTransfer = 1 << 12
};

inline TransportCapability operator|(TransportCapability a, TransportCapability b) noexcept {
    return static_cast<TransportCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline TransportCapability operator&(TransportCapability a, TransportCapability b) noexcept {
    return static_cast<TransportCapability>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool hasCapability(TransportCapability caps, TransportCapability flag) noexcept {
    return (static_cast<uint32_t>(caps) & static_cast<uint32_t>(flag)) != 0;
}

struct TransportStatistics {
    uint64_t bytesRead{0};
    uint64_t bytesWritten{0};
    uint64_t readOperations{0};
    uint64_t writeOperations{0};
    uint32_t reconnectCount{0};
    uint32_t timeoutCount{0};
    uint32_t errorCount{0};
    double averageLatency{0.0};
    double peakThroughput{0.0};
};

struct TransportEndpoint {
    TransportType type{TransportType::Unknown};
    std::string address;
    uint16_t port{0};
    uint16_t vendorId{0};
    uint16_t productId{0};
    std::string serialNumber;
    std::string description;
};

struct TransportProgress {
    uint64_t transferredBytes{0};
    uint64_t totalBytes{0};
    double percentage{0.0};
    std::chrono::milliseconds elapsed{0};
    std::chrono::milliseconds eta{0};
};

struct TransferResult {
    ErrorCode error{ErrorCode::Success};
    size_t bytesTransferred{0};
    std::chrono::milliseconds duration{0};
};

struct TransportConfig {
    std::chrono::milliseconds timeout{5000};
    bool reconnect{false};
    int retryCount{3};
    size_t bufferSize{65536};
    bool asynchronous{false};
    bool keepAlive{false};
    std::chrono::milliseconds pollingInterval{100};
};

using CompletionCallback = std::function<void(ErrorCode error, size_t bytes)>;
using ProgressCallback = std::function<void(const TransportProgress& progress)>;

} // namespace transport
} // namespace mbootcore
