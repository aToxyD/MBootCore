#include "mbootcore/core/protocols/sahara/SaharaStateMachine.hpp"

namespace mbootcore {

namespace {

struct StateImpl : public IState {
    SaharaStateId m_id;
    std::string_view m_name;

    StateImpl(SaharaStateId id, std::string_view name) : m_id(id), m_name(name) {}

    std::string_view name() const noexcept override { return m_name; }
    uint32_t id() const noexcept override { return static_cast<uint32_t>(m_id); }
};

const StateImpl s_states[] = {
    {SaharaStateId::Disconnected,       "Disconnected"},
    {SaharaStateId::WaitingForHello,    "WaitingForHello"},
    {SaharaStateId::Handshake,          "Handshake"},
    {SaharaStateId::Ready,              "Ready"},
    {SaharaStateId::ReadingImage,       "ReadingImage"},
    {SaharaStateId::SendingData,        "SendingData"},
    {SaharaStateId::WaitingEndImage,    "WaitingEndImage"},
    {SaharaStateId::SendingDone,        "SendingDone"},
    {SaharaStateId::WaitingDoneResp,    "WaitingDoneResp"},
    {SaharaStateId::SendingReset,       "SendingReset"},
    {SaharaStateId::WaitingResetResp,   "WaitingResetResp"},
    {SaharaStateId::Error,              "Error"},
    {SaharaStateId::Finished,           "Finished"},
};

const Transition s_transitions[] = {
    {0,  1,  0,  "Disconnected → WaitingForHello"},
    {1,  2,  1,  "HelloReceived → Handshake"},
    {1,  11, 11, "Timeout → Error"},
    {2,  3,  2,  "Handshake → Ready"},
    {3,  4,  3,  "Ready → ReadingImage"},
    {4,  5,  4,  "ReadingImage → SendingData"},
    {5,  3,  0,  "SendingData → Ready"},
    {3,  6,  5,  "Ready → WaitingEndImage"},
    {6,  7,  6,  "WaitingEndImage → SendingDone"},
    {7,  8,  7,  "SendingDone → WaitingDoneResp"},
    {8,  1,  0,  "WaitingDoneResp → WaitingForHello (pending)"},
    {8,  12, 0,  "WaitingDoneResp → Finished (complete)"},
    {3,  9,  8,  "Ready → SendingReset"},
    {9,  10, 9,  "SendingReset → WaitingResetResp"},
    {10, 12, 0,  "WaitingResetResp → Finished"},
    {1,  11, 10, "ErrorOccurred → Error"},
    {2,  11, 10, "ErrorOccurred → Error"},
    {3,  11, 10, "ErrorOccurred → Error"},
    {4,  11, 10, "ErrorOccurred → Error"},
    {5,  11, 10, "ErrorOccurred → Error"},
    {6,  11, 10, "ErrorOccurred → Error"},
    {7,  11, 10, "ErrorOccurred → Error"},
    {0,  11, 12, "Cancel → Error"},
    {1,  11, 12, "Cancel → Error"},
    {2,  11, 12, "Cancel → Error"},
    {3,  11, 12, "Cancel → Error"},
    {4,  11, 12, "Cancel → Error"},
    {5,  11, 12, "Cancel → Error"},
    {6,  11, 12, "Cancel → Error"},
    {7,  11, 12, "Cancel → Error"},
    {8,  11, 12, "Cancel → Error"},
    {9,  11, 12, "Cancel → Error"},
    {10, 11, 12, "Cancel → Error"},
};

} // namespace

SaharaStateMachine::SaharaStateMachine() {
    for (const auto& t : s_transitions) {
        m_transitions.push_back(t);
    }
}

const IState& SaharaStateMachine::currentState() const {
    auto idx = static_cast<size_t>(m_currentState);
    if (idx < std::size(s_states)) {
        return s_states[idx];
    }
    return s_states[0];
}

Result<void> SaharaStateMachine::transition(uint32_t event) {
    auto oldId = static_cast<uint32_t>(m_currentState);

    for (const auto& t : m_transitions) {
        if (t.fromState == oldId && t.event == event) {
            auto newId = static_cast<SaharaStateId>(t.toState);
            m_currentState = newId;
            if (m_callback) {
                auto& oldState = s_states[static_cast<size_t>(oldId)];
                auto& newState = s_states[static_cast<size_t>(newId)];
                m_callback(oldState, newState, event);
            }
            return {};
        }
    }

    return ErrorCode::InvalidState;
}

bool SaharaStateMachine::canTransition(uint32_t event) const noexcept {
    auto curId = static_cast<uint32_t>(m_currentState);
    for (const auto& t : m_transitions) {
        if (t.fromState == curId && t.event == event) {
            return true;
        }
    }
    return false;
}

void SaharaStateMachine::reset() noexcept {
    m_currentState = SaharaStateId::WaitingForHello;
}

const std::vector<Transition>& SaharaStateMachine::allowedTransitions() const {
    return m_transitions;
}

void SaharaStateMachine::onStateChanged(StateCallback callback) {
    m_callback = std::move(callback);
}

} // namespace mbootcore
