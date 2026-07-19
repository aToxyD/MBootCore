#pragma once

#include "mbootcore/domain/IStateMachine.hpp"

namespace mbootcore {

enum class SaharaStateId : uint32_t {
    Disconnected       = 0,
    WaitingForHello    = 1,
    Handshake          = 2,
    Ready              = 3,
    ReadingImage       = 4,
    SendingData        = 5,
    WaitingEndImage    = 6,
    SendingDone        = 7,
    WaitingDoneResp    = 8,
    SendingReset       = 9,
    WaitingResetResp   = 10,
    Error              = 11,
    Finished           = 12,
};

enum class SaharaEvent : uint32_t {
    HelloReceived       = 1,
    HelloResponseSent   = 2,
    ReadDataReceived    = 3,
    DataSent            = 4,
    EndImageReceived    = 5,
    DoneReceived        = 6,
    DoneResponseSent    = 7,
    ResetRequested      = 8,
    ResetResponseRecv   = 9,
    ErrorOccurred       = 10,
    Timeout             = 11,
    Cancel              = 12,
};

class SaharaStateMachine : public IStateMachine {
public:
    SaharaStateMachine();

    const IState& currentState() const override;
    Result<void> transition(uint32_t event) override;
    bool canTransition(uint32_t event) const noexcept override;
    void reset() noexcept override;
    const std::vector<Transition>& allowedTransitions() const override;
    void onStateChanged(StateCallback callback) override;

private:
    SaharaStateId m_currentState{SaharaStateId::WaitingForHello};
    std::vector<Transition> m_transitions;
    StateCallback m_callback;
};

} // namespace mbootcore
