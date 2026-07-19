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
 * @brief Abstract interface for UDP socket backends (connected datagram mode).
 *
 * UDP backends operate in connected mode (bind + connect) so that send()/recv()
 * use the connected remote address. This eliminates the need for per-call addressing.
 *
 * Implementations must be thread-safe unless documented otherwise.
 */
class IUdpBackend {
public:
    virtual ~IUdpBackend() = default;

    /// @brief Returns true if this backend type is available on the current platform.
    virtual bool isAvailable() const noexcept = 0;

    /**
     * @brief Binds to a local address and connects to a remote UDP endpoint.
     *
     * After a successful open(), send() and recv() use the configured remote address.
     *
     * @param localAddress  Local IP/address to bind to (empty for any).
     * @param localPort     Local port to bind to (0 for auto-assign).
     * @param remoteAddress Remote IP/address to connect to.
     * @param remotePort    Remote port number.
     * @param timeout       Maximum time to wait for the operation.
     * @param broadcast     Enable SO_BROADCAST option (default false).
     * @return Result<void> Ok on success, error code on failure.
     */
    virtual Result<void> open(const std::string& localAddress, uint16_t localPort,
                               const std::string& remoteAddress, uint16_t remotePort,
                               std::chrono::milliseconds timeout,
                               bool broadcast = false) = 0;

    /// @brief Closes the UDP socket. Safe to call if not connected.
    virtual void close() noexcept = 0;

    /// @brief Returns true if the socket is connected (open has been called).
    virtual bool isConnected() const noexcept = 0;

    /**
     * @brief Sends a datagram to the connected remote endpoint.
     *
     * @param data      Pointer to the data buffer.
     * @param size      Number of bytes to send.
     * @param timeout   Maximum time to wait for the send to complete.
     * @return Result<size_t> Number of bytes sent, or error code.
     */
    virtual Result<size_t> send(const uint8_t* data, size_t size,
                                 std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Receives a datagram from the connected remote endpoint.
     *
     * Blocks until data is available or the timeout expires.
     *
     * @param buffer    Destination buffer (must be at least @p size bytes).
     * @param size      Maximum number of bytes to receive.
     * @param timeout   Maximum time to wait for data.
     * @return Result<size_t> Number of bytes received (may be less than @p size on timeout).
     */
    virtual Result<size_t> recv(uint8_t* buffer, size_t size,
                                 std::chrono::milliseconds timeout) = 0;

    /// @brief Cancels any in-progress send or recv operation.
    virtual void cancel() noexcept = 0;

    /// @brief Returns a human-readable name identifying the backend implementation.
    virtual std::string backendName() const noexcept = 0;

    /// @brief Sets or clears the logger used for diagnostic output.
    virtual void setLogger(ILogger* logger) noexcept = 0;
};

} // namespace network
} // namespace transport
} // namespace mbootcore
