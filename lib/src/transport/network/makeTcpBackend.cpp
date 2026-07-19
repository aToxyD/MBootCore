#include <mbootcore/transport/network/makeTcpBackend.hpp>

#ifdef MBOOTCORE_HAVE_WIN32_TCP
#include "WinSockTcpBackend.hpp"
#elif defined(MBOOTCORE_HAVE_POSIX_TCP)
#include "PosixTcpBackend.hpp"
#endif

namespace mbootcore {
namespace transport {
namespace network {

std::unique_ptr<ITcpBackend> makeTcpBackend(ILogger* logger) {
#ifdef MBOOTCORE_HAVE_WIN32_TCP
    return std::make_unique<WinSockTcpBackend>(logger);
#elif defined(MBOOTCORE_HAVE_POSIX_TCP)
    return std::make_unique<PosixTcpBackend>(logger);
#else
    (void)logger;
    return nullptr;
#endif
}

} // namespace network
} // namespace transport
} // namespace mbootcore
