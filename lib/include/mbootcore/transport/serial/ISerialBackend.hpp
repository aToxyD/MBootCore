#pragma once

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <chrono>

namespace mbootcore {
namespace transport {
namespace serial {

/**
 * @brief Abstract interface for serial communication backends.
 *
 * Defines the platform-independent contract implemented by native
 * (Win32Serial, PosixTermios) backends.
 *
 * Implementations must be thread-safe unless documented otherwise.
 * All public methods that can fail return Result<T>.
 */
class ISerialBackend {
public:
    virtual ~ISerialBackend() = default;

    /// @brief Returns true if this backend type is available on the current platform.
    virtual bool isAvailable() const noexcept = 0;

    /**
     * @brief Opens a serial port.
     *
     * @param portName      System port identifier (e.g. "COM3" or "/dev/ttyUSB0").
     * @param baudRate      Communication speed in bps (e.g. 115200).
     * @param dataBits      Number of data bits (5, 6, 7, or 8).
     * @param stopBits      Number of stop bits (1 or 2).
     * @param parity        Parity setting: "none", "even", "odd", "space", "mark".
     * @param flowControl   Flow control: "none", "hardware"/"rts/cts", "software"/"xon/xoff".
     * @param readBufferSize  Internal receive buffer size (default 65536).
     * @return Result<void> Ok on success, TransportBackendUnavailable or error code on failure.
     */
    virtual Result<void> open(const std::string& portName, int baudRate,
                               int dataBits, int stopBits,
                               const std::string& parity,
                               const std::string& flowControl,
                               size_t readBufferSize = 65536) = 0;

    /// @brief Closes the serial port. Safe to call even if already closed.
    virtual void close() noexcept = 0;

    /// @brief Returns true if the port is currently open.
    virtual bool isOpen() const noexcept = 0;

    /**
     * @brief Writes data to the serial port.
     *
     * @param data      Pointer to the data buffer.
     * @param size      Number of bytes to write.
     * @param timeout   Maximum time to wait for the write to complete.
     * @return Result<size_t> Number of bytes written, or error code.
     */
    virtual Result<size_t> write(const uint8_t* data, size_t size,
                                  std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Reads data from the serial port.
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

    /// @brief Flushes the port's internal transmit buffers.
    virtual Result<void> flush() = 0;

    /// @brief Returns a human-readable name identifying the backend implementation.
    virtual std::string backendName() const noexcept = 0;

    /// @brief Sets or clears the logger used for diagnostic output.
    virtual void setLogger(ILogger* logger) noexcept = 0;
};

} // namespace serial
} // namespace transport
} // namespace mbootcore
