#pragma once

#include <cstdint>

namespace mbootcore::protocol {

class StatusCode {
public:
    StatusCode() noexcept
        : m_kind(Kind::Success)
        , m_reason(0)
    {}

    static constexpr StatusCode success() noexcept
    {
        return StatusCode{Kind::Success, 0};
    }

    static constexpr StatusCode failure(uint32_t reason) noexcept
    {
        return StatusCode{Kind::Failure, reason};
    }

    constexpr bool isSuccess() const noexcept { return m_kind == Kind::Success; }
    constexpr bool isFailure() const noexcept { return m_kind == Kind::Failure; }

    constexpr uint32_t reason() const noexcept { return m_reason; }

    constexpr bool operator==(const StatusCode& other) const noexcept
    {
        return m_kind == other.m_kind && m_reason == other.m_reason;
    }
    constexpr bool operator!=(const StatusCode& other) const noexcept
    {
        return !(*this == other);
    }

private:
    enum class Kind : uint8_t { Success, Failure };

    constexpr StatusCode(Kind kind, uint32_t reason) noexcept
        : m_kind(kind)
        , m_reason(reason)
    {}

    Kind     m_kind;
    uint32_t m_reason;
};

} // namespace mbootcore::protocol
