#include "PosixTcpBackend.hpp"

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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

PosixTcpBackend::PosixTcpBackend(ILogger* logger)
    : m_logger(logger) {
}

PosixTcpBackend::~PosixTcpBackend() {
    close();
}

bool PosixTcpBackend::isAvailable() const noexcept {
    return true;
}

namespace {

int setNonBlocking(int fd, bool nonBlocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    if (nonBlocking)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

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

Result<void> PosixTcpBackend::open(const std::string& host, uint16_t port,
                                    bool keepAlive,
                                    std::chrono::milliseconds timeout) {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr = std::to_string(port);
    int ret = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        if (m_logger) m_logger->error("PosixTcpBackend",
            "getaddrinfo failed for " + host + ": " + gai_strerror(ret));
        return ErrorCode::InvalidArgument;
    }

    m_fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_fd < 0) {
        freeaddrinfo(res);
        return ErrorCode::TransportBackendUnavailable;
    }
    setNonBlocking(m_fd, true);

    int optval = keepAlive ? 1 : 0;
    setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    optval = 1;
    setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));

    ::connect(m_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(m_fd, &writeSet);

    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);

    ret = select(m_fd + 1, nullptr, &writeSet, nullptr, &tv);
    if (ret <= 0) {
        int soError = 0;
        socklen_t len = sizeof(soError);
        getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &soError, &len);
        ::close(m_fd);
        m_fd = -1;
        if (m_logger) m_logger->error("PosixTcpBackend",
            "Connection to " + host + ":" + std::to_string(port) + " failed: " +
            std::strerror(soError ? soError : (ret == 0 ? ETIMEDOUT : errno)));
        return ErrorCode::TransportBackendUnavailable;
    }

    int soError = 0;
    socklen_t len = sizeof(soError);
    getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &soError, &len);
    if (soError != 0) {
        ::close(m_fd);
        m_fd = -1;
        if (m_logger) m_logger->error("PosixTcpBackend",
            "Connection to " + host + ":" + std::to_string(port) + " failed: " +
            std::strerror(soError));
        return ErrorCode::TransportBackendUnavailable;
    }

    setNonBlocking(m_fd, false);

    if (m_logger) {
        m_logger->info("PosixTcpBackend",
            "Connected to " + host + ":" + std::to_string(port));
    }
    return {};
}

void PosixTcpBackend::close() noexcept {
    if (m_fd >= 0) {
        ::shutdown(m_fd, SHUT_RDWR);
        ::close(m_fd);
        m_fd = -1;
    }
}

bool PosixTcpBackend::isConnected() const noexcept {
    return m_fd >= 0;
}

Result<size_t> PosixTcpBackend::write(const uint8_t* data, size_t size,
                                       std::chrono::milliseconds timeout) {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;

    size_t totalWritten = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (totalWritten < size) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!waitForSocket(m_fd, true, remaining)) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        ssize_t written = ::send(m_fd, data + totalWritten, size - totalWritten, 0);
        if (written < 0) {
            if (errno == EINTR) continue;
            return ErrorCode::TransportWriteFailed;
        }
        totalWritten += static_cast<size_t>(written);
    }

    return totalWritten;
}

Result<size_t> PosixTcpBackend::read(uint8_t* buffer, size_t size,
                                      std::chrono::milliseconds timeout) {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;

    size_t totalRead = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (totalRead < size) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!waitForSocket(m_fd, false, remaining)) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        ssize_t bytesRead = ::recv(m_fd, buffer + totalRead, size - totalRead, 0);
        if (bytesRead < 0) {
            if (errno == EINTR) continue;
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

void PosixTcpBackend::cancel() noexcept {
    if (m_fd >= 0) {
        ::shutdown(m_fd, SHUT_RDWR);
    }
}

Result<void> PosixTcpBackend::flush() {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;
    if (!waitForSocket(m_fd, true, std::chrono::milliseconds(1000))) {
        return ErrorCode::TransportTimeout;
    }
    return {};
}

} // namespace network
} // namespace transport
} // namespace mbootcore
