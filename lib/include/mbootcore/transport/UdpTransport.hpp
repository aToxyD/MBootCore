#pragma once

#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <memory>

namespace mbootcore {
namespace transport {

/**
 * @brief ITransport-based UDP datagram wrapper.
 *
 * Uses an internal IUdpBackend (created via makeUdpBackend())
 * to provide platform-independent connected-UDP communication
 * through the ITransport interface.
 *
 * Operates in connected mode: send() and read() use the remote
 * address configured at construction time.
 *
 * Thread safety: const methods are thread-safe by convention.
 * Mutating methods are not safe for concurrent access.
 */
class UdpTransport : public ITransport {
public:
    /**
     * @brief Constructs a UdpTransport with the given endpoint configuration.
     *
     * @param localAddr   Local address to bind to (empty for any).
     * @param localPort   Local port (0 for auto-assign).
     * @param remoteAddr  Remote address to connect to.
     * @param remotePort  Remote port number.
     * @param broadcast   Enable SO_BROADCAST (default false).
     * @param logger      Optional logger for diagnostics.
     */
    UdpTransport(const std::string& localAddr = {}, uint16_t localPort = 0,
                 const std::string& remoteAddr = {}, uint16_t remotePort = 0,
                 bool broadcast = false, ILogger* logger = nullptr);
    ~UdpTransport() override;

    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override;
    Result<size_t> write(const ByteBuffer& data, std::chrono::milliseconds timeout) override;
    Result<size_t> read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                        std::chrono::milliseconds timeout) override;
    Result<void> sendZLP(std::chrono::milliseconds timeout) override;
    std::string_view name() const noexcept override { return "UdpTransport"; }
    void cancel() noexcept override;

    TransportType transportType() const noexcept override { return TransportType::UDP; }
    TransportCapability capabilities() const noexcept override {
        return TransportCapability::Read | TransportCapability::Write
             | TransportCapability::Timeout | TransportCapability::Cancellation
             | TransportCapability::Configurable | TransportCapability::Observable;
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
    std::string m_localAddr, m_remoteAddr;
    uint16_t m_localPort, m_remotePort;
    bool m_broadcast;
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
