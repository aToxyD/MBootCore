#pragma once

#include <mbootcore/transport/serial/ISerialBackend.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <memory>

namespace mbootcore {
namespace transport {
namespace serial {

/**
 * @brief Creates the best available serial backend for the current platform.
 *
 * Selection order (compile-time):
 *   1. Win32SerialBackend  (Windows, native)
 *   2. PosixSerialBackend  (Linux/macOS, native)
 *
 * @param logger Optional logger for diagnostics (may be nullptr).
 * @return A unique_ptr to the backend, or nullptr if no serial backend is available.
 */
std::unique_ptr<ISerialBackend> makeSerialBackend(ILogger* logger = nullptr);

} // namespace serial
} // namespace transport
} // namespace mbootcore
