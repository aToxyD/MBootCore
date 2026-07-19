#pragma once

#include "Error.hpp"

#include <functional>
#include <string_view>
#include <vector>

namespace mbootcore {

class IState {
public:
    virtual ~IState() = default;
    virtual std::string_view name() const noexcept = 0;
    virtual uint32_t id() const noexcept = 0;
};

struct Transition {
    uint32_t fromState;
    uint32_t toState;
    uint32_t event;
    std::string_view description;
};

class IStateMachine {
public:
    virtual ~IStateMachine() = default;

    virtual const IState& currentState() const = 0;
    virtual Result<void> transition(uint32_t event) = 0;
    virtual bool canTransition(uint32_t event) const noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual const std::vector<Transition>& allowedTransitions() const = 0;

    using StateCallback = std::function<void(const IState& oldState,
                                              const IState& newState,
                                              uint32_t event)>;
    virtual void onStateChanged(StateCallback callback) = 0;
};

} // namespace mbootcore
