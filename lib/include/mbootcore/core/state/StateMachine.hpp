#pragma once

#include "mbootcore/domain/IStateMachine.hpp"

#include <unordered_map>
#include <vector>
#include <string>

namespace mbootcore {

class GenericStateMachine : public IStateMachine {
public:
    GenericStateMachine();

    void addState(uint32_t id, std::string_view name);
    void addTransition(uint32_t from, uint32_t to, uint32_t event,
                       std::string_view description);
    void setInitialState(uint32_t id);

    const IState& currentState() const override;
    Result<void> transition(uint32_t event) override;
    bool canTransition(uint32_t event) const noexcept override;
    void reset() noexcept override;
    const std::vector<Transition>& allowedTransitions() const override;
    void onStateChanged(StateCallback callback) override;

private:
    struct StateValue : public IState {
        uint32_t m_id;
        std::string m_name;
        StateValue() : m_id(0), m_name() {}
        StateValue(uint32_t id, std::string name) : m_id(id), m_name(std::move(name)) {}
        std::string_view name() const noexcept override { return m_name; }
        uint32_t id() const noexcept override { return m_id; }
    };

    std::unordered_map<uint32_t, StateValue> m_states;
    std::vector<Transition> m_transitions;
    uint32_t m_currentState{0};
    uint32_t m_initialState{0};
    StateCallback m_callback;
};

} // namespace mbootcore
