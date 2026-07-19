#pragma once

#include <cstdint>
#include <functional>

namespace mbootcore::protocol {

class TransactionId {
public:
    constexpr TransactionId() noexcept = default;

    explicit constexpr TransactionId(uint64_t value) noexcept
        : m_value(value)
    {}

    constexpr uint64_t value() const noexcept { return m_value; }

    constexpr bool operator==(const TransactionId& other) const noexcept { return m_value == other.m_value; }
    constexpr bool operator!=(const TransactionId& other) const noexcept { return m_value != other.m_value; }
    constexpr bool operator<(const TransactionId& other) const noexcept  { return m_value < other.m_value; }

private:
    uint64_t m_value = 0;
};

} // namespace mbootcore::protocol

namespace std {

template<>
struct hash<mbootcore::protocol::TransactionId> {
    size_t operator()(const mbootcore::protocol::TransactionId& id) const noexcept {
        return hash<uint64_t>{}(id.value());
    }
};

} // namespace std
