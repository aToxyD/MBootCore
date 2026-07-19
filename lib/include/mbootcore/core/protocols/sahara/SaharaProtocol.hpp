#pragma once

#include "mbootcore/domain/IProtocol.hpp"
#include "mbootcore/domain/ITransport.hpp"
#include "mbootcore/domain/ILogger.hpp"

#include "mbootcore/core/protocols/sahara/SaharaStateMachine.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"

#include <vector>
#include <atomic>

namespace mbootcore {

/// Sahara-specific negotiated version information from HELLO_REQ handshake.
struct SaharaNegotiatedVersion {
    uint32_t current{0};    ///< Negotiated version (e.g. 2 for V2, 3 for V3)
    uint32_t supported{0};  ///< Minimum version the device supports (from HELLO_REQ)

    bool isCompatible(uint32_t hostVersion) const noexcept {
        return hostVersion >= supported && hostVersion <= current;
    }
};

class SaharaProtocol : public IProtocol {
public:
    explicit SaharaProtocol(ITransport& transport, ILogger& logger);

    std::string_view name() const noexcept override { return "Sahara"; }
    ProtocolPhase phase() const noexcept override { return m_phase; }

    Result<void> handshake() override;
    Result<void> uploadProgrammer(const ByteBuffer& programmerData) override;
    Result<void> reset() override;
    void cancel() noexcept override;
    void resetState() noexcept override;

    IStateMachine& stateMachine() override { return m_stateMachine; }
    std::vector<ErrorCode> errors() const noexcept override { return m_errors; }
    void onProgress(ProgressCallback callback) override { m_progressCallback = callback; }

    uint32_t negotiatedVersion() const noexcept { return m_negotiatedVersion; }
    const DeviceInfo& deviceInfo() const noexcept { return m_deviceInfo; }

    static constexpr uint32_t DefaultVersion = 3;
    static constexpr uint32_t DefaultVersionSupported = 2;
    static constexpr uint32_t HelloResponseSuccess = 0;
    static constexpr uint32_t DefaultTimeoutMs = 5000;

private:
    Result<std::unique_ptr<IPacket>> receivePacket(uint32_t expectedCmd);
    Result<void> sendPacket(const IPacket& packet);
    Result<void> performV2Discovery();

    ITransport& m_transport;
    ILogger& m_logger;
    SaharaPacketSerializer m_serializer;
    SaharaPacketParser m_parser;
    SaharaStateMachine m_stateMachine;
    ProtocolPhase m_phase{ProtocolPhase::Idle};
    std::vector<ErrorCode> m_errors;
    ProgressCallback m_progressCallback;
    std::atomic<bool> m_cancelled{false};
    uint32_t m_negotiatedVersion{0};
    DeviceInfo m_deviceInfo{};
    bool m_v2DiscoveryAttempted{false};
};

} // namespace mbootcore
