#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <chrono>

namespace mbootcore {
namespace transport {
namespace usb {

/// @brief Status of a USB transfer operation.
enum class TransferStatus : uint8_t {
    Pending = 0,
    Completed,
    Failed,
    Cancelled,
    Timeout,
    DeviceDisconnected
};

/// @brief Represents a USB transfer (bulk or control) with timing and status.
struct UsbTransfer {
    uint8_t endpoint{0};
    std::vector<uint8_t> buffer;
    size_t transferredBytes{0};
    TransferStatus status{TransferStatus::Pending};
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds timeout{5000};
    bool isRead{true};

    /// @brief Returns elapsed time in milliseconds since the transfer started.
    double elapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            now - startTime).count();
    }

    /// @brief Calculates throughput in bytes per second from transferred bytes and elapsed time.
    double throughputBps() const {
        auto ms = elapsedMs();
        if (ms <= 0.0) return 0.0;
        return (static_cast<double>(transferredBytes) * 1000.0) / ms;
    }
};

} // namespace usb
} // namespace transport
} // namespace mbootcore
