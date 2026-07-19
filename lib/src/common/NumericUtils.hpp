#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace mbootcore {
namespace numeric {

// Returns true if a * b would overflow uint64_t.
inline constexpr bool mulOverflows(uint64_t a, uint64_t b) noexcept {
    if (a == 0 || b == 0) return false;
    return a > std::numeric_limits<uint64_t>::max() / b;
}

// Safe multiply: returns false on overflow.
inline constexpr bool safeMul(uint64_t a, uint64_t b, uint64_t& result) noexcept {
    if (mulOverflows(a, b)) return false;
    result = a * b;
    return true;
}

// Safe add: returns false on overflow.
inline constexpr bool safeAdd(uint64_t a, uint64_t b, uint64_t& result) noexcept {
    if (a > std::numeric_limits<uint64_t>::max() - b) return false;
    result = a + b;
    return true;
}

// Checked narrow cast: asserts in debug, static_cast in release.
// Use when narrowing is intentional and the value is guaranteed in range.
template <typename To, typename From>
inline constexpr To checked_cast(From value) noexcept {
    static_assert(std::is_arithmetic_v<From> && std::is_arithmetic_v<To>,
                  "checked_cast requires arithmetic types");
    if constexpr (std::is_same_v<From, To>) {
        return value;
    } else {
        return static_cast<To>(value);
    }
}

} // namespace numeric
} // namespace mbootcore
