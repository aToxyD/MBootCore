#pragma once

#include <mbootcore/domain/ITransport.hpp>
#include <mbootcore/transport/TransportTypes.hpp>
#include <memory>
#include <deque>
#include <random>
#include <chrono>
#include <atomic>
#include <thread>

namespace mbootcore {
namespace transport {

/**
 * @brief Configuration for simulating real-world transport conditions.
 *
 * Used by VirtualUsbTransport to emulate latency, bandwidth limits,
 * packet loss, corruption, disconnections, and other fault scenarios.
 */
struct VirtualTransportConfig {
    bool simulateLatency{false};
    std::chrono::milliseconds minLatency{10};
    std::chrono::milliseconds maxLatency{50};

    bool simulateBandwidth{false};
    size_t bandwidthBytesPerSec{102400};

    bool simulatePacketLoss{false};
    double packetLossRate{0.05};

    bool simulateCorruption{false};
    double corruptionRate{0.01};

    bool simulateDisconnect{false};
    std::chrono::milliseconds disconnectAfter{0};
    bool autoReconnect{false};

    bool simulateTimeout{false};
    std::chrono::milliseconds timeoutDelay{10000};

    bool simulateFragmentation{false};
    size_t fragmentSize{64};

    bool simulateDuplication{false};
    double duplicationRate{0.02};

    bool simulateTruncation{false};
    double truncationRate{0.01};

    size_t maxQueueSize{1024};
    bool failOnOpen{false};
    bool failOnWrite{false};
    bool failOnRead{false};
};

/**
 * @brief Configuration for targeted fault injection during testing.
 *
 * Enables specific failure modes like read/write timeouts, disconnect
 * during transfer, packet corruption, and delayed delivery.
 */
struct FaultInjectionConfig {
    bool enableReadTimeout{false};
    bool enableWriteTimeout{false};
    bool enableDisconnectDuringTransfer{false};
    bool enableReconnectDuringTransfer{false};
    bool enablePacketCorruption{false};
    bool enableCrcMismatch{false};
    bool enableRandomByteCorruption{false};
    bool enablePacketDuplication{false};
    bool enablePacketTruncation{false};
    bool enableDelayedPackets{false};

    std::chrono::milliseconds timeoutDelay{5000};
    size_t failAfterBytes{0};
    double corruptionProbability{0.1};
    std::chrono::milliseconds maxPacketDelay{1000};
};

/**
 * @brief Simulated USB transport for testing without real hardware.
 *
 * Supports configurable latency, bandwidth limits, packet loss,
 * corruption, disconnection, and fragmentation simulation.
 *
 * Data is injected via injectData() and read via the ITransport read() API.
 * Disconnections can be triggered manually or simulated automatically.
 */
class VirtualUsbTransport : public ITransport {
public:
    VirtualUsbTransport();
    explicit VirtualUsbTransport(const VirtualTransportConfig& config);
    ~VirtualUsbTransport() override;

    void setConfig(const VirtualTransportConfig& cfg) { m_virtConfig = cfg; }
    VirtualTransportConfig& virtConfig() noexcept { return m_virtConfig; }
    const VirtualTransportConfig& virtConfig() const noexcept { return m_virtConfig; }

    void setFaultConfig(const FaultInjectionConfig& cfg) { m_faultConfig = cfg; }
    FaultInjectionConfig& faultConfig() noexcept { return m_faultConfig; }
    const FaultInjectionConfig& faultConfig() const noexcept { return m_faultConfig; }

    /// @brief Injects data into the virtual read queue.
    void injectData(const ByteBuffer& data);

    /// @brief Triggers a simulated disconnect.
    void triggerDisconnect();

    /// @brief Triggers a simulated reconnect.
    void triggerReconnect();

    /// @brief Returns true if a simulated disconnect is active.
    bool isSimulatedDisconnect() const noexcept { return m_simDisconnected; }

    /// @brief Returns total bytes injected via injectData().
    size_t totalBytesInjected() const noexcept { return m_injected; }

    /// @brief Returns total bytes read via read().
    size_t totalBytesRead() const noexcept { return m_bytesRead; }

    /// @brief Returns total bytes written via write().
    size_t totalBytesWritten() const noexcept { return m_bytesWritten; }

    TransportType transportType() const noexcept override { return TransportType::Virtual; }
    TransportCapability capabilities() const noexcept override {
        return TransportCapability::Read | TransportCapability::Write
             | TransportCapability::Timeout | TransportCapability::Cancellation
             | TransportCapability::Configurable | TransportCapability::Observable
             | TransportCapability::BulkTransfer | TransportCapability::ControlTransfer
             | TransportCapability::Reconnectable;
    }
    TransportState state() const noexcept override;
    TransportStatistics statistics() const override { return m_stats; }
    const TransportConfig& config() const override { return m_config; }
    void setConfig(const TransportConfig& cfg) override { m_config = cfg; }
    TransportEndpoint endpoint() const override;

    Result<void> open() override;
    void close() noexcept override;
    bool isOpen() const noexcept override;
    Result<size_t> write(const ByteBuffer& data, std::chrono::milliseconds timeout) override;
    Result<size_t> read(ByteBuffer& buffer, size_t minBytes, size_t maxBytes,
                        std::chrono::milliseconds timeout) override;
    Result<void> sendZLP(std::chrono::milliseconds timeout) override;
    std::string_view name() const noexcept override { return "VirtualUsbTransport"; }
    void cancel() noexcept override;
    Result<void> flush() override;
    Result<void> reset() override;
    Result<void> reconnect() override;

private:
    VirtualTransportConfig m_virtConfig;
    FaultInjectionConfig m_faultConfig;
    TransportConfig m_config;
    TransportStatistics m_stats;
    TransportState m_state{TransportState::Closed};
    std::atomic<bool> m_cancelled{false};
    std::atomic<bool> m_simDisconnected{false};

    std::deque<ByteBuffer> m_readQueue;
    std::mt19937 m_rng;
    size_t m_injected{0};
    size_t m_bytesRead{0};
    size_t m_bytesWritten{0};
    std::chrono::steady_clock::time_point m_startTime;

    void applyLatency();
    bool shouldDrop();
    bool shouldCorrupt();
    void maybeCorrupt(ByteBuffer& data);
    bool shouldDuplicate();
    bool shouldTruncate();
    void applyFaults();
    bool checkFaultInjection();
};

} // namespace transport
} // namespace mbootcore