#include "mbootcore/core/state/StateMachine.hpp"

namespace mbootcore {

GenericStateMachine::GenericStateMachine() = default;

void GenericStateMachine::addState(uint32_t id, std::string_view name) {
    m_states.try_emplace(id, StateValue{id, std::string(name)});
}

void GenericStateMachine::addTransition(uint32_t from, uint32_t to, uint32_t event,
                                        std::string_view description) {
    m_transitions.push_back({from, to, event, description});
}

void GenericStateMachine::setInitialState(uint32_t id) {
    m_initialState = id;
    m_currentState = id;
}

const IState& GenericStateMachine::currentState() const {
    static StateValue fallback{0, "Unknown"};
    auto it = m_states.find(m_currentState);
    if (it != m_states.end()) {
        return it->second;
    }
    return fallback;
}

Result<void> GenericStateMachine::transition(uint32_t event) {
    auto oldState = m_currentState;

    for (const auto& t : m_transitions) {
        if (t.fromState == oldState && t.event == event) {
            m_currentState = t.toState;
            if (m_callback) {
                auto& old = m_states.at(oldState);
                auto& next = m_states.at(t.toState);
                m_callback(old, next, event);
            }
            return {};
        }
    }

    return ErrorCode::InvalidState;
}

bool GenericStateMachine::canTransition(uint32_t event) const noexcept {
    for (const auto& t : m_transitions) {
        if (t.fromState == m_currentState && t.event == event) {
            return true;
        }
    }
    return false;
}

void GenericStateMachine::reset() noexcept {
    m_currentState = m_initialState;
}

const std::vector<Transition>& GenericStateMachine::allowedTransitions() const {
    return m_transitions;
}

void GenericStateMachine::onStateChanged(StateCallback callback) {
    m_callback = std::move(callback);
}

} // namespace mbootcore
