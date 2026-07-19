#include "PosixUdpBackend.hpp"

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>

namespace mbootcore {
namespace transport {
namespace network {

namespace {

bool waitForSocket(int fd, bool forWrite, std::chrono::milliseconds timeout) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);

    int ret;
    if (forWrite) {
        ret = select(fd + 1, nullptr, &set, nullptr, &tv);
    } else {
        ret = select(fd + 1, &set, nullptr, nullptr, &tv);
    }
    return ret > 0;
}

} // anonymous namespace

PosixUdpBackend::PosixUdpBackend(ILogger* logger)
    : m_logger(logger) {}

PosixUdpBackend::~PosixUdpBackend() {
    close();
}

bool PosixUdpBackend::isAvailable() const noexcept {
    return true;
}

Result<void> PosixUdpBackend::open(const std::string& localAddress, uint16_t localPort,
                                    const std::string& remoteAddress, uint16_t remotePort,
                                    std::chrono::milliseconds timeout,
                                    bool broadcast) {
    close();

    m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0) {
        if (m_logger) m_logger->error("PosixUdpBackend", "socket() failed");
        return ErrorCode::TransportBackendUnavailable;
    }

    if (broadcast) {
        int optval = 1;
        if (::setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
            if (m_logger) m_logger->warn("PosixUdpBackend", "setsockopt SO_BROADCAST failed");
        }
    }

    struct sockaddr_in localAddr = {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(localPort);

    if (localAddress.empty() || localAddress == "0.0.0.0") {
        localAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, localAddress.c_str(), &localAddr.sin_addr) != 1) {
            if (m_logger) m_logger->error("PosixUdpBackend",
                "Invalid local address: " + localAddress);
            ::close(m_fd);
            m_fd = -1;
            return ErrorCode::InvalidArgument;
        }
    }

    if (::bind(m_fd, reinterpret_cast<struct sockaddr*>(&localAddr), sizeof(localAddr)) < 0) {
        if (m_logger) m_logger->error("PosixUdpBackend",
            "bind() to " + localAddress + ":" + std::to_string(localPort) +
            " failed: " + std::strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return ErrorCode::TransportBackendUnavailable;
    }

    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    std::string portStr = std::to_string(remotePort);
    int ret = getaddrinfo(remoteAddress.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        if (m_logger) m_logger->error("PosixUdpBackend",
            "getaddrinfo failed for " + remoteAddress);
        ::close(m_fd);
        m_fd = -1;
        return ErrorCode::InvalidArgument;
    }

    int flags = fcntl(m_fd, F_GETFL, 0);
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

    ret = ::connect(m_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (ret < 0 && errno != EINPROGRESS) {
        if (m_logger) m_logger->error("PosixUdpBackend",
            "connect() to " + remoteAddress + ":" + portStr + " failed");
        ::close(m_fd);
        m_fd = -1;
        return ErrorCode::TransportBackendUnavailable;
    }

    if (ret < 0) {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_fd, &writeSet);

        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
        tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);

        ret = select(m_fd + 1, nullptr, &writeSet, nullptr, &tv);
        if (ret <= 0) {
            if (m_logger) m_logger->error("PosixUdpBackend",
                "connect() to " + remoteAddress + ":" + portStr + " timed out");
            ::close(m_fd);
            m_fd = -1;
            return ErrorCode::TransportTimeout;
        }
    }

    fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK);

    if (m_logger) {
        m_logger->info("PosixUdpBackend",
            "UDP connected " + localAddress + ":" + std::to_string(localPort) +
            " -> " + remoteAddress + ":" + portStr);
    }
    return {};
}

void PosixUdpBackend::close() noexcept {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool PosixUdpBackend::isConnected() const noexcept {
    return m_fd >= 0;
}

Result<size_t> PosixUdpBackend::send(const uint8_t* data, size_t size,
                                      std::chrono::milliseconds timeout) {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;

    if (!waitForSocket(m_fd, true, timeout)) {
        return ErrorCode::TransportTimeout;
    }

    ssize_t sent = ::send(m_fd, data, size, 0);
    if (sent < 0) {
        if (errno == EINTR) return send(data, size, timeout);
        return ErrorCode::TransportWriteFailed;
    }

    return static_cast<size_t>(sent);
}

Result<size_t> PosixUdpBackend::recv(uint8_t* buffer, size_t size,
                                      std::chrono::milliseconds timeout) {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;

    if (!waitForSocket(m_fd, false, timeout)) {
        return ErrorCode::TransportTimeout;
    }

    ssize_t bytesRead = ::recv(m_fd, buffer, size, 0);
    if (bytesRead < 0) {
        if (errno == EINTR) return recv(buffer, size, timeout);
        return ErrorCode::TransportReadFailed;
    }
    if (bytesRead == 0) {
        return ErrorCode::TransportNotOpen;
    }

    return static_cast<size_t>(bytesRead);
}

void PosixUdpBackend::cancel() noexcept {
    if (m_fd >= 0) {
        ::shutdown(m_fd, SHUT_RDWR);
    }
}

} // namespace network
} // namespace transport
} // namespace mbootcore
