#ifdef _WIN32
#include "WinSockTcpBackend.hpp"
#include "platform/WinSockRuntime.hpp"
#include "platform/UniqueSocket.hpp"

#include <cstring>
#include <sstream>

namespace mbootcore {
namespace transport {
namespace network {

namespace {

bool waitForSocket(SOCKET sock, bool forWrite, std::chrono::milliseconds timeout) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);

    struct timeval tv;
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    int ret;
    if (forWrite) {
        ret = ::select(0, nullptr, &set, nullptr, &tv);
    } else {
        ret = ::select(0, &set, nullptr, nullptr, &tv);
    }
    return ret > 0;
}

int setNonBlocking(SOCKET sock, bool nonBlocking) {
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(sock, FIONBIO, &mode);
}

} // anonymous namespace

WinSockTcpBackend::WinSockTcpBackend(ILogger* logger)
    : m_logger(logger) {
}

WinSockTcpBackend::~WinSockTcpBackend() {
    close();
}

bool WinSockTcpBackend::isAvailable() const noexcept {
    return m_runtime.isInitialized();
}

Result<void> WinSockTcpBackend::open(const std::string& host, uint16_t port,
                                      bool keepAlive,
                                      std::chrono::milliseconds timeout) {
    if (!m_runtime.isInitialized()) {
        if (m_logger) m_logger->error("WinSockTcpBackend", "WSAStartup failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    close();

    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr = std::to_string(port);
    int ret = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        if (m_logger) m_logger->error("WinSockTcpBackend",
            "getaddrinfo failed for " + host + ": " + gai_strerror(ret));
        return ErrorCode::InvalidArgument;
    }

    platform::UniqueSocket sock(::socket(res->ai_family, res->ai_socktype, res->ai_protocol));
    if (!sock.isValid()) {
        freeaddrinfo(res);
        return ErrorCode::TransportBackendUnavailable;
    }

    setNonBlocking(sock.get(), true);

    int optval = keepAlive ? 1 : 0;
    setsockopt(sock.get(), SOL_SOCKET, SO_KEEPALIVE,
               reinterpret_cast<const char*>(&optval), sizeof(optval));
    optval = 1;
    setsockopt(sock.get(), IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&optval), sizeof(optval));

    ::connect(sock.get(), res->ai_addr, static_cast<int>(res->ai_addrlen));
    freeaddrinfo(res);

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock.get(), &writeSet);

    struct timeval tv;
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    ret = ::select(0, nullptr, &writeSet, nullptr, &tv);
    if (ret <= 0) {
        int soError = 0;
        socklen_t len = sizeof(soError);
        getsockopt(sock.get(), SOL_SOCKET, SO_ERROR,
                   reinterpret_cast<char*>(&soError), &len);
        if (m_logger) m_logger->error("WinSockTcpBackend",
            "Connection to " + host + ":" + std::to_string(port) + " failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    int soError = 0;
    socklen_t len = sizeof(soError);
    getsockopt(sock.get(), SOL_SOCKET, SO_ERROR,
               reinterpret_cast<char*>(&soError), &len);
    if (soError != 0) {
        if (m_logger) m_logger->error("WinSockTcpBackend",
            "Connection to " + host + ":" + std::to_string(port) + " failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    setNonBlocking(sock.get(), false);
    m_sock = sock.release();

    if (m_logger) {
        m_logger->info("WinSockTcpBackend",
            "Connected to " + host + ":" + std::to_string(port));
    }
    return {};
}

void WinSockTcpBackend::close() noexcept {
    if (m_sock != INVALID_SOCKET) {
        ::shutdown(m_sock, SD_BOTH);
        ::closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
}

bool WinSockTcpBackend::isConnected() const noexcept {
    return m_sock != INVALID_SOCKET;
}

Result<size_t> WinSockTcpBackend::write(const uint8_t* data, size_t size,
                                         std::chrono::milliseconds timeout) {
    if (m_sock == INVALID_SOCKET)
        return ErrorCode::TransportNotOpen;

    size_t totalWritten = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (totalWritten < size) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!waitForSocket(m_sock, true, remaining)) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        int written = ::send(m_sock,
            reinterpret_cast<const char*>(data + totalWritten),
            static_cast<int>(size - totalWritten), 0);
        if (written == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEINTR) continue;
            return ErrorCode::TransportWriteFailed;
        }
        totalWritten += static_cast<size_t>(written);
    }

    return totalWritten;
}

Result<size_t> WinSockTcpBackend::read(uint8_t* buffer, size_t size,
                                        std::chrono::milliseconds timeout) {
    if (m_sock == INVALID_SOCKET)
        return ErrorCode::TransportNotOpen;

    size_t totalRead = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (totalRead < size) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!waitForSocket(m_sock, false, remaining)) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        int bytesRead = ::recv(m_sock,
            reinterpret_cast<char*>(buffer + totalRead),
            static_cast<int>(size - totalRead), 0);
        if (bytesRead == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEINTR) continue;
            return ErrorCode::TransportReadFailed;
        }
        if (bytesRead == 0) {
            close();
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportNotOpen;
        }
        totalRead += static_cast<size_t>(bytesRead);
    }

    return totalRead;
}

void WinSockTcpBackend::cancel() noexcept {
    if (m_sock != INVALID_SOCKET) {
        ::shutdown(m_sock, SD_BOTH);
    }
}

Result<void> WinSockTcpBackend::flush() {
    if (m_sock == INVALID_SOCKET)
        return ErrorCode::TransportNotOpen;
    if (!waitForSocket(m_sock, true, std::chrono::milliseconds(1000))) {
        return ErrorCode::TransportTimeout;
    }
    return {};
}

} // namespace network
} // namespace transport
} // namespace mbootcore

#endif // _WIN32
