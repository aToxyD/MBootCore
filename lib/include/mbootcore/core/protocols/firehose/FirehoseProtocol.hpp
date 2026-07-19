#pragma once

#include "mbootcore/domain/IProtocol.hpp"
#include "mbootcore/domain/ITransport.hpp"
#include "mbootcore/domain/ILogger.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"
#include "mbootcore/core/protocols/firehose/FirehoseStreamEngine.hpp"

#include <atomic>
#include <optional>

namespace mbootcore {

struct FirehoseConfig {
    std::string memoryName{"ufs"};
    uint32_t maxPayloadSizeToTarget{1048576};
    uint32_t maxPayloadSizeFromTarget{1048576};
    uint32_t zlpAwareHost{1};
    uint32_t mode{0};
    std::chrono::milliseconds commandTimeout{5000};
    std::chrono::milliseconds chunkTimeout{10000};
    int maxConfigureRetries{3};
};

class FirehoseProtocol : public IProtocol {
public:
    FirehoseProtocol(ITransport& transport, ILogger& logger,
                     FirehoseConfig config = {});

    std::string_view name() const noexcept override { return "Firehose"; }
    ProtocolPhase phase() const noexcept override { return m_phase; }

    Result<void> handshake() override;
    Result<void> uploadProgrammer(const ByteBuffer&) override;
    Result<void> reset() override;
    void cancel() noexcept override;
    void resetState() noexcept override;

    IStateMachine& stateMachine() override { return *m_stateMachine; }
    std::vector<ErrorCode> errors() const noexcept override { return m_errors; }
    void onProgress(ProgressCallback callback) override { m_progressCallback = callback; }

    /// Firehose-specific operations (after configure).
    Result<FirehoseResponse> sendCommand(const FirehoseCommand& cmd);
    Result<FirehoseResponse> program(const ProgramCommand& cmd, const ByteBuffer& data);
    Result<ByteBuffer> read(const ReadCommand& cmd);
    Result<void> erase(const EraseCommand& cmd);
    Result<ByteBuffer> peek(const PeekCommand& cmd);
    Result<void> poke(const PokeCommand& cmd);
    Result<void> patch(const PatchCommand& cmd);
    Result<void> powerReset(const PowerCommand& cmd);
    Result<FirehoseResponse> getStorageInfo();
    Result<ByteBuffer> getSha256Digest(const GetSha256DigestCommand& cmd);
    Result<void> configureMemory(const ConfigureMemoryCommand& cmd);

    bool isConfigured() const noexcept { return m_configured; }
    const FirehoseConfig& config() const noexcept { return m_config; }

private:
    Result<void> performConfigure();
    Result<FirehoseResponse> sendXmlCommand(const std::string& xml);
    Result<FirehoseResponse> receiveResponse();

    ITransport& m_transport;
    ILogger& m_logger;
    std::unique_ptr<IStateMachine> m_stateMachine;
    FirehoseConfig m_config;
    ProtocolPhase m_phase{ProtocolPhase::Idle};
    std::vector<ErrorCode> m_errors;
    ProgressCallback m_progressCallback;
    std::atomic<bool> m_cancelled{false};
    std::unique_ptr<FirehoseStreamEngine> m_streamEngine;
    bool m_configured{false};
};

} // namespace mbootcore