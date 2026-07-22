#ifdef _WIN32
#include "WinSockUdpBackend.hpp"
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

} // anonymous namespace

WinSockUdpBackend::WinSockUdpBackend(ILogger* logger)
    : m_logger(logger) {}

WinSockUdpBackend::~WinSockUdpBackend() {
    close();
}

bool WinSockUdpBackend::isAvailable() const noexcept {
    return m_runtime.isInitialized();
}

Result<void> WinSockUdpBackend::open(const std::string& localAddress, uint16_t localPort,
                                      const std::string& remoteAddress, uint16_t remotePort,
                                      std::chrono::milliseconds timeout,
                                      bool broadcast) {
    if (!m_runtime.isInitialized()) {
        if (m_logger) m_logger->error("WinSockUdpBackend", "WSAStartup failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    close();

    platform::UniqueSocket sock(::socket(AF_INET, SOCK_DGRAM, 0));
    if (!sock.isValid()) {
        if (m_logger) m_logger->error("WinSockUdpBackend", "socket() failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    if (broadcast) {
        int optval = 1;
        if (::setsockopt(sock.get(), SOL_SOCKET, SO_BROADCAST,
                         reinterpret_cast<const char*>(&optval), sizeof(optval)) < 0) {
            if (m_logger) m_logger->warn("WinSockUdpBackend", "setsockopt SO_BROADCAST failed");
        }
    }

    struct sockaddr_in localAddr = {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(localPort);

    if (localAddress.empty() || localAddress == "0.0.0.0") {
        localAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, localAddress.c_str(), &localAddr.sin_addr) != 1) {
            if (m_logger) m_logger->error("WinSockUdpBackend",
                "Invalid local address: " + localAddress);
            return ErrorCode::InvalidArgument;
        }
    }

    if (::bind(sock.get(), reinterpret_cast<struct sockaddr*>(&localAddr),
               sizeof(localAddr)) == SOCKET_ERROR) {
        if (m_logger) m_logger->error("WinSockUdpBackend",
            "bind() to " + localAddress + ":" + std::to_string(localPort) +
            " failed: error " + std::to_string(WSAGetLastError()));
        return ErrorCode::TransportBackendUnavailable;
    }

    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    std::string portStr = std::to_string(remotePort);
    int ret = getaddrinfo(remoteAddress.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        if (m_logger) m_logger->error("WinSockUdpBackend",
            "getaddrinfo failed for " + remoteAddress);
        return ErrorCode::InvalidArgument;
    }

    u_long nonBlocking = 1;
    ioctlsocket(sock.get(), FIONBIO, &nonBlocking);

    ret = ::connect(sock.get(), res->ai_addr, static_cast<int>(res->ai_addrlen));
    freeaddrinfo(res);

    if (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        if (m_logger) m_logger->error("WinSockUdpBackend",
            "connect() to " + remoteAddress + ":" + portStr + " failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    if (ret == SOCKET_ERROR) {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock.get(), &writeSet);

        struct timeval tv;
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

        ret = ::select(0, nullptr, &writeSet, nullptr, &tv);
        if (ret <= 0) {
            if (m_logger) m_logger->error("WinSockUdpBackend",
                "connect() to " + remoteAddress + ":" + portStr + " timed out");
            return ErrorCode::TransportTimeout;
        }
    }

    nonBlocking = 0;
    ioctlsocket(sock.get(), FIONBIO, &nonBlocking);

    m_sock = sock.release();

    if (m_logger) {
        m_logger->info("WinSockUdpBackend",
            "UDP connected " + localAddress + ":" + std::to_string(localPort) +
            " -> " + remoteAddress + ":" + portStr);
    }
    return {};
}

void WinSockUdpBackend::close() noexcept {
    if (m_sock != INVALID_SOCKET) {
        ::closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
}

bool WinSockUdpBackend::isConnected() const noexcept {
    return m_sock != INVALID_SOCKET;
}

Result<size_t> WinSockUdpBackend::send(const uint8_t* data, size_t size,
                                        std::chrono::milliseconds timeout) {
    if (m_sock == INVALID_SOCKET)
        return ErrorCode::TransportNotOpen;

    if (!waitForSocket(m_sock, true, timeout)) {
        return ErrorCode::TransportTimeout;
    }

    int sent = ::send(m_sock, reinterpret_cast<const char*>(data),
                      static_cast<int>(size), 0);
    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEINTR) return send(data, size, timeout);
        return ErrorCode::TransportWriteFailed;
    }

    return static_cast<size_t>(sent);
}

Result<size_t> WinSockUdpBackend::recv(uint8_t* buffer, size_t size,
                                        std::chrono::milliseconds timeout) {
    if (m_sock == INVALID_SOCKET)
        return ErrorCode::TransportNotOpen;

    if (!waitForSocket(m_sock, false, timeout)) {
        return ErrorCode::TransportTimeout;
    }

    int bytesRead = ::recv(m_sock, reinterpret_cast<char*>(buffer),
                           static_cast<int>(size), 0);
    if (bytesRead == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEINTR) return recv(buffer, size, timeout);
        return ErrorCode::TransportReadFailed;
    }
    if (bytesRead == 0) {
        return ErrorCode::TransportNotOpen;
    }

    return static_cast<size_t>(bytesRead);
}

void WinSockUdpBackend::cancel() noexcept {
    if (m_sock != INVALID_SOCKET) {
        ::shutdown(m_sock, SD_BOTH);
    }
}

} // namespace network
} // namespace transport
} // namespace mbootcore

#endif // _WIN32
