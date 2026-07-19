#pragma once

#include <mbootcore/transport/network/ITcpBackend.hpp>

namespace mbootcore {
namespace transport {
namespace network {

class PosixTcpBackend : public ITcpBackend {
public:
    explicit PosixTcpBackend(ILogger* logger = nullptr);
    ~PosixTcpBackend() override;

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

    std::string backendName() const noexcept override { return "PosixTcpSocket"; }
    void setLogger(ILogger* logger) noexcept override { m_logger = logger; }

private:
    int m_fd{-1};
    ILogger* m_logger;
};

} // namespace network
} // namespace transport
} // namespace mbootcore
