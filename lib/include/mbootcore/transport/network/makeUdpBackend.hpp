#pragma once

#include <mbootcore/transport/network/IUdpBackend.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <memory>

namespace mbootcore {
namespace transport {
namespace network {

/**
 * @brief Creates the best available UDP backend for the current platform.
 *
 * Selection order (compile-time):
 *   1. WinSockUdpBackend  (Windows, native)
 *   2. PosixUdpBackend    (Linux/macOS, native)
 *
 * @param logger Optional logger for diagnostics (may be nullptr).
 * @return A unique_ptr to the backend, or nullptr if no UDP backend is available.
 */
std::unique_ptr<IUdpBackend> makeUdpBackend(ILogger* logger = nullptr);

} // namespace network
} // namespace transport
} // namespace mbootcore
