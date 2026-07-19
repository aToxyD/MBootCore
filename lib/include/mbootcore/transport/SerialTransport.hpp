#pragma once

#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <memory>

namespace mbootcore {
namespace transport {

/**
 * @brief ITransport-based serial communication wrapper.
 *
 * Uses an internal ISerialBackend (created via makeSerialBackend())
 * to provide platform-independent serial I/O through the ITransport interface.
 *
 * Thread safety: const methods are thread-safe by convention.
 * Mutating methods are not safe for concurrent access.
 */
class SerialTransport : public ITransport {
public:
    /**
     * @brief Constructs a SerialTransport with the given serial configuration.
     *
     * @param portName      System port name (e.g. "COM3" or "/dev/ttyUSB0").
     * @param baudRate      Communication speed in bps (default 115200).
     * @param dataBits      Data bits (default 8).
     * @param stopBits      Stop bits (default 1).
     * @param parity        Parity: "none", "even", "odd", "space", "mark".
     * @param flowControl   Flow control: "none", "hardware", "software".
     * @param logger        Optional logger for diagnostics.
     */
    SerialTransport(const std::string& portName, int baudRate = 115200,
                    int dataBits = 8, int stopBits = 1,
                    const std::string& parity = "none",
                    const std::string& flowControl = "none",
                    ILogger* logger = nullptr);
    ~SerialTransport() override;

    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override;
    Result<size_t> write(const ByteBuffer& data, std::chrono::milliseconds timeout) override;
    Result<size_t> read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                        std::chrono::milliseconds timeout) override;
    Result<void> sendZLP(std::chrono::milliseconds timeout) override;
    std::string_view name() const noexcept override { return "SerialTransport"; }
    void cancel() noexcept override;

    TransportType transportType() const noexcept override { return TransportType::Serial; }
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
    std::string m_portName;
    int m_baudRate, m_dataBits, m_stopBits;
    std::string m_parity, m_flowControl;
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
