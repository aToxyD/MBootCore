#pragma once

#include <cstdint>
#include <functional>

namespace mbootcore::protocol {

class CommandId {
public:
    constexpr CommandId() noexcept = default;

    explicit constexpr CommandId(uint32_t value) noexcept
        : m_value(value)
    {}

    constexpr uint32_t value() const noexcept { return m_value; }

    constexpr bool operator==(const CommandId& other) const noexcept { return m_value == other.m_value; }
    constexpr bool operator!=(const CommandId& other) const noexcept { return m_value != other.m_value; }
    constexpr bool operator<(const CommandId& other) const noexcept  { return m_value < other.m_value; }

private:
    uint32_t m_value = 0;
};

} // namespace mbootcore::protocol

namespace std {

template<>
struct hash<mbootcore::protocol::CommandId> {
    size_t operator()(const mbootcore::protocol::CommandId& id) const noexcept {
        return hash<uint32_t>{}(id.value());
    }
};

} // namespace std
