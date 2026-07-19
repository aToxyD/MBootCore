#pragma once

#ifdef _WIN32
#include <winsock2.h>

namespace mbootcore {
namespace platform {

class UniqueSocket {
public:
    explicit UniqueSocket(SOCKET sock = INVALID_SOCKET);
    ~UniqueSocket();

    UniqueSocket(UniqueSocket&& other) noexcept;
    UniqueSocket& operator=(UniqueSocket&& other) noexcept;

    UniqueSocket(const UniqueSocket&) = delete;
    UniqueSocket& operator=(const UniqueSocket&) = delete;

    SOCKET get() const noexcept { return m_sock; }
    SOCKET release() noexcept;
    void reset(SOCKET sock = INVALID_SOCKET);

    bool isValid() const noexcept { return m_sock != INVALID_SOCKET; }
    explicit operator bool() const noexcept { return isValid(); }

private:
    SOCKET m_sock;
};

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
