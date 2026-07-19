#ifdef _WIN32
#include "Win32SerialBackend.hpp"
#include "platform/Win32Handle.hpp"

#include <sstream>
#include <cstring>

namespace mbootcore {
namespace transport {
namespace serial {

namespace {

int baudRateToDCB(int baudRate) {
    switch (baudRate) {
        case 110: return CBR_110;
        case 300: return CBR_300;
        case 600: return CBR_600;
        case 1200: return CBR_1200;
        case 2400: return CBR_2400;
        case 4800: return CBR_4800;
        case 9600: return CBR_9600;
        case 14400: return CBR_14400;
        case 19200: return CBR_19200;
        case 38400: return CBR_38400;
        case 57600: return CBR_57600;
        case 115200: return CBR_115200;
        case 128000: return CBR_128000;
        case 256000: return CBR_256000;
        default: return CBR_115200;
    }
}

BYTE dataBitsToByte(int dataBits) {
    switch (dataBits) {
        case 5: return 5;
        case 6: return 6;
        case 7: return 7;
        case 8: return 8;
        default: return 8;
    }
}

BYTE stopBitsToByte(int stopBits) {
    return (stopBits == 2) ? TWOSTOPBITS : ONESTOPBIT;
}

BYTE parityToByte(const std::string& parity) {
    if (parity == "even") return EVENPARITY;
    if (parity == "odd") return ODDPARITY;
    if (parity == "space") return SPACEPARITY;
    if (parity == "mark") return MARKPARITY;
    return NOPARITY;
}

} // anonymous namespace

Win32SerialBackend::Win32SerialBackend(ILogger* logger)
    : m_logger(logger) {
}

Win32SerialBackend::~Win32SerialBackend() {
    close();
}

bool Win32SerialBackend::isAvailable() const noexcept {
    return true;
}

Result<void> Win32SerialBackend::open(const std::string& portName, int baudRate,
                                       int dataBits, int stopBits,
                                       const std::string& parity,
                                       const std::string& flowControl,
                                       size_t readBufferSize) {
    close();

    std::string winPath = "\\\\.\\" + portName;

    platform::Win32Handle handle(::CreateFileA(
        winPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));

    if (!handle.isValid()) {
        std::string err = "Failed to open " + portName + ": error " +
                          std::to_string(GetLastError());
        if (m_logger) m_logger->error("Win32SerialBackend", err);
        return ErrorCode::TransportBackendUnavailable;
    }

    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!::GetCommState(handle.get(), &dcb)) {
        if (m_logger) m_logger->error("Win32SerialBackend",
            "GetCommState failed: error " + std::to_string(GetLastError()));
        return ErrorCode::TransportBackendUnavailable;
    }

    dcb.BaudRate = baudRateToDCB(baudRate);
    dcb.ByteSize = dataBitsToByte(dataBits);
    dcb.StopBits = stopBitsToByte(stopBits);
    dcb.Parity = parityToByte(parity);
    dcb.fParity = (parity != "none") ? TRUE : FALSE;

    if (flowControl == "hardware" || flowControl == "rts/cts") {
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    } else if (flowControl == "xon/xoff") {
        dcb.fInX = TRUE;
        dcb.fOutX = TRUE;
    } else {
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fInX = FALSE;
        dcb.fOutX = FALSE;
    }

    if (!::SetCommState(handle.get(), &dcb)) {
        if (m_logger) m_logger->error("Win32SerialBackend",
            "SetCommState failed: error " + std::to_string(GetLastError()));
        return ErrorCode::TransportBackendUnavailable;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;

    if (!::SetCommTimeouts(handle.get(), &timeouts)) {
        if (m_logger) m_logger->error("Win32SerialBackend",
            "SetCommTimeouts failed: error " + std::to_string(GetLastError()));
        return ErrorCode::TransportBackendUnavailable;
    }

    if (readBufferSize > 0) {
        if (!::SetupComm(handle.get(), static_cast<DWORD>(readBufferSize),
                         static_cast<DWORD>(readBufferSize))) {
            if (m_logger) m_logger->warn("Win32SerialBackend",
                "SetupComm failed: error " + std::to_string(GetLastError()));
        }
    }

    m_handle = handle.release();

    if (m_logger) {
        m_logger->info("Win32SerialBackend",
            "Opened " + portName + " @ " + std::to_string(baudRate) + " baud");
    }
    return {};
}

void Win32SerialBackend::close() noexcept {
    if (m_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    m_cancelled = false;
}

bool Win32SerialBackend::isOpen() const noexcept {
    return m_handle != INVALID_HANDLE_VALUE;
}

Result<size_t> Win32SerialBackend::write(const uint8_t* data, size_t size,
                                          std::chrono::milliseconds timeout) {
    if (m_handle == INVALID_HANDLE_VALUE)
        return ErrorCode::TransportNotOpen;
    if (m_cancelled)
        return ErrorCode::TransportAsyncCancelled;

    DWORD written = 0;
    size_t totalWritten = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (totalWritten < size) {
        if (m_cancelled)
            return ErrorCode::TransportAsyncCancelled;

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalWritten > 0) return totalWritten;
            return ErrorCode::TransportTimeout;
        }

        DWORD toWrite = static_cast<DWORD>(
            (std::min)(size - totalWritten, static_cast<size_t>(4096)));

        if (!::WriteFile(m_handle, data + totalWritten, toWrite, &written, nullptr)) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) continue;
            return ErrorCode::TransportWriteFailed;
        }
        if (written == 0) break;
        totalWritten += static_cast<size_t>(written);
    }

    return totalWritten;
}

Result<size_t> Win32SerialBackend::read(uint8_t* buffer, size_t size,
                                         std::chrono::milliseconds timeout) {
    if (m_handle == INVALID_HANDLE_VALUE)
        return ErrorCode::TransportNotOpen;
    if (m_cancelled)
        return ErrorCode::TransportAsyncCancelled;

    DWORD totalRead = 0;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (static_cast<size_t>(totalRead) < size) {
        if (m_cancelled)
            return ErrorCode::TransportAsyncCancelled;

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (totalRead > 0) return totalRead;
            return ErrorCode::TransportTimeout;
        }

        DWORD toRead = static_cast<DWORD>(
            (std::min)(size - static_cast<size_t>(totalRead), static_cast<size_t>(4096)));

        if (!::ReadFile(m_handle, buffer + totalRead, toRead, &totalRead, nullptr)) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) continue;
            return ErrorCode::TransportReadFailed;
        }
        if (totalRead == 0) break;
    }

    return static_cast<size_t>(totalRead);
}

void Win32SerialBackend::cancel() noexcept {
    m_cancelled = true;
    if (m_handle != INVALID_HANDLE_VALUE) {
        ::PurgeComm(m_handle, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);
    }
}

Result<void> Win32SerialBackend::flush() {
    if (m_handle == INVALID_HANDLE_VALUE)
        return ErrorCode::TransportNotOpen;
    if (!::FlushFileBuffers(m_handle)) {
        return ErrorCode::TransportWriteFailed;
    }
    return {};
}

} // namespace serial
} // namespace transport
} // namespace mbootcore

#endif // _WIN32
