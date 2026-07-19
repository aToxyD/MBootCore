#pragma once

#include <cstdint>
#include <functional>

namespace mbootcore::protocol {

class MessageId {
public:
    constexpr MessageId() noexcept = default;

    explicit constexpr MessageId(uint64_t value) noexcept
        : m_value(value)
    {}

    constexpr uint64_t value() const noexcept { return m_value; }

    constexpr bool operator==(const MessageId& other) const noexcept { return m_value == other.m_value; }
    constexpr bool operator!=(const MessageId& other) const noexcept { return m_value != other.m_value; }
    constexpr bool operator<(const MessageId& other) const noexcept  { return m_value < other.m_value; }

private:
    uint64_t m_value = 0;
};

} // namespace mbootcore::protocol

namespace std {

template<>
struct hash<mbootcore::protocol::MessageId> {
    size_t operator()(const mbootcore::protocol::MessageId& id) const noexcept {
        return hash<uint64_t>{}(id.value());
    }
};

} // namespace std
