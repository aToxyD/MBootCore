#pragma once

#include "Types.hpp"
#include "IStateMachine.hpp"

#include <vector>
#include <functional>

namespace mbootcore {

enum class ProtocolPhase : uint8_t {
    Idle,
    Handshake,
    Transferring,
    WaitingForProgrammer,
    FirehoseReady,
    Error,
    Finished
};

class IProtocol {
public:
    virtual ~IProtocol() = default;

    virtual std::string_view name() const noexcept = 0;
    virtual ProtocolPhase phase() const noexcept = 0;

    virtual Result<void> handshake() = 0;
    virtual Result<void> uploadProgrammer(const ByteBuffer& programmerData) = 0;
    virtual Result<void> reset() = 0;
    virtual void cancel() noexcept = 0;
    virtual void resetState() noexcept = 0;

    virtual IStateMachine& stateMachine() = 0;
    virtual std::vector<ErrorCode> errors() const noexcept = 0;

    using ProgressCallback = std::function<void(size_t current, size_t total)>;
    virtual void onProgress(ProgressCallback callback) = 0;
};

} // namespace mbootcore
