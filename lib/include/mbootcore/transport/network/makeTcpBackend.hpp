#pragma once

#include <mbootcore/transport/network/ITcpBackend.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <memory>

namespace mbootcore {
namespace transport {
namespace network {

/**
 * @brief Creates the best available TCP backend for the current platform.
 *
 * Selection order (compile-time):
 *   1. WinSockTcpBackend  (Windows, native)
 *   2. PosixTcpBackend    (Linux/macOS, native)
 *
 * @param logger Optional logger for diagnostics (may be nullptr).
 * @return A unique_ptr to the backend, or nullptr if no TCP backend is available.
 */
std::unique_ptr<ITcpBackend> makeTcpBackend(ILogger* logger = nullptr);

} // namespace network
} // namespace transport
} // namespace mbootcore
