#ifdef _WIN32
#include "UniqueSocket.hpp"

namespace mbootcore {
namespace platform {

UniqueSocket::UniqueSocket(SOCKET sock)
    : m_sock(sock) {}

UniqueSocket::~UniqueSocket() {
    if (isValid()) {
        closesocket(m_sock);
    }
}

UniqueSocket::UniqueSocket(UniqueSocket&& other) noexcept
    : m_sock(other.m_sock) {
    other.m_sock = INVALID_SOCKET;
}

UniqueSocket& UniqueSocket::operator=(UniqueSocket&& other) noexcept {
    if (this != &other) {
        if (isValid()) {
            closesocket(m_sock);
        }
        m_sock = other.m_sock;
        other.m_sock = INVALID_SOCKET;
    }
    return *this;
}

SOCKET UniqueSocket::release() noexcept {
    SOCKET s = m_sock;
    m_sock = INVALID_SOCKET;
    return s;
}

void UniqueSocket::reset(SOCKET sock) {
    if (isValid()) {
        closesocket(m_sock);
    }
    m_sock = sock;
}

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
