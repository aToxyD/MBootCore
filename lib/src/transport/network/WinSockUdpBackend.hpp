#pragma once

#include <mbootcore/transport/network/IUdpBackend.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "platform/WinSockRuntime.hpp"
#endif

namespace mbootcore {
namespace transport {
namespace network {

#ifdef _WIN32
class WinSockUdpBackend : public IUdpBackend {
public:
    explicit WinSockUdpBackend(ILogger* logger = nullptr);
    ~WinSockUdpBackend() override;

    bool isAvailable() const noexcept override;

    Result<void> open(const std::string& localAddress, uint16_t localPort,
                       const std::string& remoteAddress, uint16_t remotePort,
                       std::chrono::milliseconds timeout,
                       bool broadcast = false) override;
    void close() noexcept override;
    bool isConnected() const noexcept override;

    Result<size_t> send(const uint8_t* data, size_t size,
                         std::chrono::milliseconds timeout) override;
    Result<size_t> recv(uint8_t* buffer, size_t size,
                         std::chrono::milliseconds timeout) override;

    void cancel() noexcept override;

    std::string backendName() const noexcept override { return "WinSockUdp"; }
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
