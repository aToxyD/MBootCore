#include <mbootcore/transport/network/makeUdpBackend.hpp>

#ifdef MBOOTCORE_HAVE_WIN32_UDP
#include "WinSockUdpBackend.hpp"
#elif defined(MBOOTCORE_HAVE_POSIX_UDP)
#include "PosixUdpBackend.hpp"
#endif

namespace mbootcore {
namespace transport {
namespace network {

std::unique_ptr<IUdpBackend> makeUdpBackend(ILogger* logger) {
#ifdef MBOOTCORE_HAVE_WIN32_UDP
    return std::make_unique<WinSockUdpBackend>(logger);
#elif defined(MBOOTCORE_HAVE_POSIX_UDP)
    return std::make_unique<PosixUdpBackend>(logger);
#else
    (void)logger;
    return nullptr;
#endif
}

} // namespace network
} // namespace transport
} // namespace mbootcore
