#pragma once

#include <mbootcore/transport/network/ITcpBackend.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "platform/WinSockRuntime.hpp"
#endif

namespace mbootcore {
namespace transport {
namespace network {

#ifdef _WIN32
class WinSockTcpBackend : public ITcpBackend {
public:
    explicit WinSockTcpBackend(ILogger* logger = nullptr);
    ~WinSockTcpBackend() override;

    bool isAvailable() const noexcept override;

    Result<void> open(const std::string& host, uint16_t port,
                       bool keepAlive,
                       std::chrono::milliseconds timeout) override;
    void close() noexcept override;
    bool isConnected() const noexcept override;

    Result<size_t> write(const uint8_t* data, size_t size,
                          std::chrono::milliseconds timeout) override;
    Result<size_t> read(uint8_t* buffer, size_t size,
                         std::chrono::milliseconds timeout) override;

    void cancel() noexcept override;
    Result<void> flush() override;

    std::string backendName() const noexcept override { return "WinSockTcp"; }
    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

private:
    platform::WinSockRuntime m_runtime;
    ILogger* m_logger{nullptr};
    SOCKET m_sock{INVALID_SOCKET};
};
#endif // _WIN32

} // namespace network
} // namespace transport
} // namespace mbootcore
