#include <mbootcore/transport/serial/makeSerialBackend.hpp>

#ifdef MBOOTCORE_HAVE_WIN32_SERIAL
#include "Win32SerialBackend.hpp"
#elif defined(MBOOTCORE_HAVE_POSIX_SERIAL)
#include "PosixSerialBackend.hpp"
#endif

namespace mbootcore {
namespace transport {
namespace serial {

std::unique_ptr<ISerialBackend> makeSerialBackend(ILogger* logger) {
#ifdef MBOOTCORE_HAVE_WIN32_SERIAL
    return std::make_unique<Win32SerialBackend>(logger);
#elif defined(MBOOTCORE_HAVE_POSIX_SERIAL)
    return std::make_unique<PosixSerialBackend>(logger);
#else
    (void)logger;
    return nullptr;
#endif
}

} // namespace serial
} // namespace transport
} // namespace mbootcore
