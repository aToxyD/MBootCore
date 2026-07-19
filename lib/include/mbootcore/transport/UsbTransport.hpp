#pragma once

#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <mbootcore/transport/usb/UsbBackend.hpp>
#include <memory>

namespace mbootcore {
namespace transport {

/**
 * @brief ITransport-based USB device wrapper.
 *
 * Uses an internal UsbBackend (created via makeUsbBackend())
 * to provide USB bulk/control transfers through the ITransport interface.
 *
 * The backend can be injected externally via setBackend() for testing
 * or custom configuration.
 *
 * Thread safety: const methods are thread-safe by convention.
 * Mutating methods are not safe for concurrent access.
 */
class UsbTransport : public ITransport {
public:
    /**
     * @brief Constructs a UsbTransport targeting a specific USB device.
     *
     * @param vendorId         USB vendor ID (e.g. 0x05C6 for Qualcomm).
     * @param productId        USB product ID.
     * @param interfaceNumber  Interface number to claim (default 0).
     * @param bulkInEp         Bulk IN endpoint address (default 0x81).
     * @param bulkOutEp        Bulk OUT endpoint address (default 0x01).
     * @param logger           Optional logger for diagnostics.
     */
    UsbTransport(uint16_t vendorId, uint16_t productId,
                 int interfaceNumber = 0, int bulkInEp = 0x81, int bulkOutEp = 0x01,
                 ILogger* logger = nullptr);
    ~UsbTransport() override;

    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override;
    Result<size_t> write(const ByteBuffer& data, std::chrono::milliseconds timeout) override;
    Result<size_t> read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                        std::chrono::milliseconds timeout) override;
    Result<void> sendZLP(std::chrono::milliseconds timeout) override;
    std::string_view name() const noexcept override { return "UsbTransport"; }
    void cancel() noexcept override;

    TransportType transportType() const noexcept override { return TransportType::USB; }
    TransportCapability capabilities() const noexcept override {
        return TransportCapability::Read | TransportCapability::Write
             | TransportCapability::Timeout | TransportCapability::Cancellation
             | TransportCapability::Configurable | TransportCapability::Observable
             | TransportCapability::BulkTransfer | TransportCapability::ControlTransfer
             | TransportCapability::Hotplug;
    }
    TransportState state() const noexcept override;
    TransportStatistics statistics() const override { return m_stats; }
    const TransportConfig& config() const override { return m_config; }
    void setConfig(const TransportConfig& cfg) override { m_config = cfg; }
    Result<void> flush() override;
    Result<void> reset() override;
    Result<void> reconnect() override;
    TransportEndpoint endpoint() const override;

    /// @brief Injects an external USB backend (used for testing or custom backends).
    void setBackend(std::unique_ptr<usb::UsbBackend> backend);

private:
    uint16_t m_vendorId, m_productId;
    int m_interfaceNumber, m_bulkInEp, m_bulkOutEp;
    ILogger* m_logger;
    TransportConfig m_config;
    TransportStatistics m_stats;
    TransportState m_state{TransportState::Closed};
    bool m_cancelled{false};
    std::unique_ptr<usb::UsbBackend> m_backend;
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    void updateLatency(std::chrono::microseconds us);
    void updatePeakThroughput(size_t bytes, std::chrono::microseconds us);
};

} // namespace transport
} // namespace mbootcore
