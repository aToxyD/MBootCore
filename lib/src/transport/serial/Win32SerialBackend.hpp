#pragma once

#include <mbootcore/transport/serial/ISerialBackend.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mbootcore {
namespace transport {
namespace serial {

#ifdef _WIN32
class Win32SerialBackend : public ISerialBackend {
public:
    explicit Win32SerialBackend(ILogger* logger = nullptr);
    ~Win32SerialBackend() override;

    bool isAvailable() const noexcept override;

    Result<void> open(const std::string& portName, int baudRate,
                       int dataBits, int stopBits,
                       const std::string& parity,
                       const std::string& flowControl,
                       size_t readBufferSize) override;
    void close() noexcept override;
    bool isOpen() const noexcept override;

    Result<size_t> write(const uint8_t* data, size_t size,
                          std::chrono::milliseconds timeout) override;
    Result<size_t> read(uint8_t* buffer, size_t size,
                         std::chrono::milliseconds timeout) override;

    void cancel() noexcept override;
    Result<void> flush() override;

    std::string backendName() const noexcept override { return "Win32Serial"; }
    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

private:
    HANDLE m_handle{INVALID_HANDLE_VALUE};
    ILogger* m_logger;
    bool m_cancelled{false};
};
#endif // _WIN32

} // namespace serial
} // namespace transport
} // namespace mbootcore
