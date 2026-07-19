#pragma once

#include <mbootcore/runtime/OrchestrationTypes.hpp>
#include <mbootcore/domain/Error.hpp>

#include <mutex>
#include <type_traits>
#include <utility>

namespace mbootcore {
namespace runtime {

struct RuntimeState;

class OperationContext {
public:
    OperationContext(RuntimeState& state, std::mutex& opMutex) noexcept
        : m_state(&state), m_opMutex(&opMutex) {}

    template<typename Fn>
    auto invoke(Fn&& fn, LockMode lockMode = LockMode::Exclusive)
        -> std::invoke_result_t<Fn>
    {
        using ResultType = std::invoke_result_t<Fn>;

        if (!m_state->initialized)
            return ResultType::Error(ErrorCode::InvalidState);

        std::unique_lock<std::mutex> lock(*m_opMutex, std::defer_lock);
        if (lockMode == LockMode::Exclusive) lock.lock();

        if (m_state->cancelled)
            return ResultType::Error(ErrorCode::Cancelled);

        return std::forward<Fn>(fn)();
    }

private:
    RuntimeState* m_state;
    std::mutex* m_opMutex;
};

} // namespace runtime
} // namespace mbootcore
