#pragma once

#include <mbootcore/transport/network/IUdpBackend.hpp>

namespace mbootcore {
namespace transport {
namespace network {

class PosixUdpBackend : public IUdpBackend {
public:
    explicit PosixUdpBackend(ILogger* logger = nullptr);
    ~PosixUdpBackend() override;

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

    std::string backendName() const noexcept override { return "PosixUdpSocket"; }
    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

private:
    int m_fd{-1};
    ILogger* m_logger;
};

} // namespace network
} // namespace transport
} // namespace mbootcore
