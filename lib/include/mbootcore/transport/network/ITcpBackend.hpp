#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <chrono>

namespace mbootcore {
namespace transport {
namespace network {

/**
 * @brief Abstract interface for TCP socket backends.
 *
 * Defines the platform-independent contract implemented by native
 * (WinSockTcp, PosixTcpSocket) backends.
 *
 * Implementations must be thread-safe unless documented otherwise.
 */
class ITcpBackend {
public:
    virtual ~ITcpBackend() = default;

    /// @brief Returns true if this backend type is available on the current platform.
    virtual bool isAvailable() const noexcept = 0;

    /**
     * @brief Connects to a TCP server.
     *
     * @param host      Remote hostname or IP address.
     * @param port      Remote port number.
     * @param keepAlive Enable TCP keep-alive probes.
     * @param timeout   Maximum time to wait for the connection to establish.
     * @return Result<void> Ok on success, error code on failure.
     */
    virtual Result<void> open(const std::string& host, uint16_t port,
                               bool keepAlive,
                               std::chrono::milliseconds timeout) = 0;

    /// @brief Closes the TCP connection. Safe to call if not connected.
    virtual void close() noexcept = 0;

    /// @brief Returns true if the socket is currently connected.
    virtual bool isConnected() const noexcept = 0;

    /**
     * @brief Writes data to the TCP socket.
     *
     * @param data      Pointer to the data buffer.
     * @param size      Number of bytes to write.
     * @param timeout   Maximum time to wait for the write to complete.
     * @return Result<size_t> Number of bytes written, or error code.
     */
    virtual Result<size_t> write(const uint8_t* data, size_t size,
                                  std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Reads data from the TCP socket.
     *
     * Blocks until at least one byte is available or the timeout expires.
     *
     * @param buffer    Destination buffer (must be at least @p size bytes).
     * @param size      Maximum number of bytes to read.
     * @param timeout   Maximum time to wait for data.
     * @return Result<size_t> Number of bytes read (may be less than @p size on timeout).
     */
    virtual Result<size_t> read(uint8_t* buffer, size_t size,
                                 std::chrono::milliseconds timeout) = 0;

    /// @brief Cancels any in-progress read or write operation.
    virtual void cancel() noexcept = 0;

    /// @brief Flushes the socket's internal transmit buffer.
    virtual Result<void> flush() = 0;

    /// @brief Returns a human-readable name identifying the backend implementation.
    virtual std::string backendName() const noexcept = 0;

    /// @brief Sets or clears the logger used for diagnostic output.
    virtual void setLogger(ILogger* logger) noexcept = 0;
};

} // namespace network
} // namespace transport
} // namespace mbootcore
