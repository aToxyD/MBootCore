#pragma once

#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <memory>

namespace mbootcore {
namespace transport {

/**
 * @brief ITransport-based TCP client wrapper.
 *
 * Uses an internal ITcpBackend (created via makeTcpBackend())
 * to provide platform-independent TCP connectivity through the ITransport interface.
 *
 * Thread safety: const methods are thread-safe by convention.
 * Mutating methods are not safe for concurrent access.
 */
class TcpTransport : public ITransport {
public:
    /**
     * @brief Constructs a TcpTransport for the given remote endpoint.
     *
     * @param host      Remote hostname or IP address.
     * @param port      Remote port number.
     * @param keepAlive Enable TCP keep-alive probes (default true).
     * @param logger    Optional logger for diagnostics.
     */
    TcpTransport(const std::string& host, uint16_t port,
                 bool keepAlive = true, ILogger* logger = nullptr);
    ~TcpTransport() override;

    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override;
    Result<size_t> write(const ByteBuffer& data, std::chrono::milliseconds timeout) override;
    Result<size_t> read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                        std::chrono::milliseconds timeout) override;
    Result<void> sendZLP(std::chrono::milliseconds timeout) override;
    std::string_view name() const noexcept override { return "TcpTransport"; }
    void cancel() noexcept override;

    TransportType transportType() const noexcept override { return TransportType::TCP; }
    TransportCapability capabilities() const noexcept override {
        return TransportCapability::Read | TransportCapability::Write
             | TransportCapability::Timeout | TransportCapability::Cancellation
             | TransportCapability::Configurable | TransportCapability::Observable
             | TransportCapability::Reconnectable;
    }
    TransportState state() const noexcept override;
    TransportStatistics statistics() const override { return m_stats; }
    const TransportConfig& config() const override { return m_config; }
    void setConfig(const TransportConfig& cfg) override { m_config = cfg; }
    Result<void> flush() override;
    Result<void> reset() override;
    Result<void> reconnect() override;
    TransportEndpoint endpoint() const override;

private:
    std::string m_host;
    uint16_t m_port;
    bool m_keepAlive;
    ILogger* m_logger;
    TransportConfig m_config;
    TransportStatistics m_stats;
    TransportState m_state{TransportState::Closed};
    bool m_cancelled{false};
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace transport
} // namespace mbootcore
