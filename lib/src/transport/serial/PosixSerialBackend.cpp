#include "PosixSerialBackend.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <cerrno>
#include <cstring>
#include <sstream>

namespace mbootcore {
namespace transport {
namespace serial {

namespace {

speed_t baudRateToSpeed(int baudRate) {
    switch (baudRate) {
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
        default: return B115200;
    }
}

int parityToTermios(const std::string& parity) {
    if (parity == "even") return PARENB;
    if (parity == "odd") return PARENB | PARODD;
    if (parity == "space") return 0; // not standard termios
    if (parity == "mark") return 0;  // not standard termios
    return 0; // none
}

int stopBitsToTermios(int stopBits) {
    return (stopBits == 2) ? CSTOPB : 0;
}

int dataBitsToTermios(int dataBits) {
    switch (dataBits) {
        case 5: return CS5;
        case 6: return CS6;
        case 7: return CS7;
        default: return CS8;
    }
}

} // anonymous namespace

PosixSerialBackend::PosixSerialBackend(ILogger* logger)
    : m_logger(logger) {
}

PosixSerialBackend::~PosixSerialBackend() {
    close();
}

bool PosixSerialBackend::isAvailable() const noexcept {
    return true;
}

Result<void> PosixSerialBackend::open(const std::string& portName, int baudRate,
                                       int dataBits, int stopBits,
                                       const std::string& parity,
                                       const std::string& flowControl,
                                       size_t readBufferSize) {
    (void)readBufferSize;

    if (m_fd >= 0) {
        close();
    }

    m_fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        std::string err = "Failed to open " + portName + ": " + std::strerror(errno);
        if (m_logger) m_logger->error("PosixSerialBackend", err);
        return ErrorCode::TransportBackendUnavailable;
    }

    struct termios tio;
    if (tcgetattr(m_fd, &tio) < 0) {
        std::string err = "tcgetattr failed: " + std::string(std::strerror(errno));
        if (m_logger) m_logger->error("PosixSerialBackend", err);
        close();
        return ErrorCode::TransportBackendUnavailable;
    }

    cfmakeraw(&tio);
    tio.c_cflag |= CREAD | CLOCAL;
    tio.c_cflag &= ~HUPCL;

    cfsetispeed(&tio, baudRateToSpeed(baudRate));
    cfsetospeed(&tio, baudRateToSpeed(baudRate));

    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= dataBitsToTermios(dataBits);

    tio.c_cflag &= ~(PARENB | PARODD);
    tio.c_cflag |= parityToTermios(parity);

    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag |= stopBitsToTermios(stopBits);

    if (flowControl == "hardware" || flowControl == "rts/cts") {
        tio.c_cflag |= CRTSCTS;
    } else {
        tio.c_cflag &= ~CRTSCTS;
    }

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1; // 100ms read timeout (deciseconds)

    if (tcsetattr(m_fd, TCSANOW, &tio) < 0) {
        std::string err = "tcsetattr failed: " + std::string(std::strerror(errno));
        if (m_logger) m_logger->error("PosixSerialBackend", err);
        close();
        return ErrorCode::TransportBackendUnavailable;
    }

    if (m_logger) {
        m_logger->info("PosixSerialBackend",
            "Opened " + portName + " @ " + std::to_string(baudRate) + " baud");
    }
    return {};
}

void PosixSerialBackend::close() noexcept {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_cancelled = false;
}

bool PosixSerialBackend::isOpen() const noexcept {
    return m_fd >= 0;
}

Result<size_t> PosixSerialBackend::write(const uint8_t* data, size_t size,
                                          std::chrono::milliseconds timeout) {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

    size_t totalWritten = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (totalWritten < size) {
        if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_fd, &writeSet);

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);
        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(remaining.count() / 1000000);
        tv.tv_usec = static_cast<suseconds_t>(remaining.count() % 1000000);

        int ret = select(m_fd + 1, nullptr, &writeSet, nullptr, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            return ErrorCode::TransportWriteFailed;
        }
        if (ret == 0) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        ssize_t written = ::write(m_fd, data + totalWritten, size - totalWritten);
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            if (errno == EINTR) continue;
            return ErrorCode::TransportWriteFailed;
        }
        totalWritten += static_cast<size_t>(written);
    }

    return totalWritten;
}

Result<size_t> PosixSerialBackend::read(uint8_t* buffer, size_t size,
                                         std::chrono::milliseconds timeout) {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;
    if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

    auto deadline = std::chrono::steady_clock::now() + timeout;
    size_t totalRead = 0;

    while (totalRead < size) {
        if (m_cancelled) return ErrorCode::TransportAsyncCancelled;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_fd, &readSet);

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);
        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(remaining.count() / 1000000);
        tv.tv_usec = static_cast<suseconds_t>(remaining.count() % 1000000);

        int ret = select(m_fd + 1, &readSet, nullptr, nullptr, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            return ErrorCode::TransportReadFailed;
        }
        if (ret == 0) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        ssize_t bytesRead = ::read(m_fd, buffer + totalRead, size - totalRead);
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            if (errno == EINTR) continue;
            return ErrorCode::TransportReadFailed;
        }
        if (bytesRead == 0) {
            break;
        }
        totalRead += static_cast<size_t>(bytesRead);
    }

    return totalRead;
}

void PosixSerialBackend::cancel() noexcept {
    m_cancelled = true;
    if (m_fd >= 0) {
        tcflush(m_fd, TCIOFLUSH);
    }
}

Result<void> PosixSerialBackend::flush() {
    if (m_fd < 0) return ErrorCode::TransportNotOpen;
    if (tcdrain(m_fd) < 0) {
        return ErrorCode::TransportWriteFailed;
    }
    return {};
}

} // namespace serial
} // namespace transport
} // namespace mbootcore
