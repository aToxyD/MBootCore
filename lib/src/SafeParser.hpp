#pragma once

#include <charconv>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace mbootcore {

struct ParseResult64 {
    uint64_t value;
    bool ok;
};

struct ParseResult32 {
    uint32_t value;
    bool ok;
};

struct ParseResult16 {
    uint16_t value;
    bool ok;
};

inline bool isWhitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline std::string_view trimView(std::string_view s) noexcept {
    while (!s.empty() && isWhitespace(s.front())) s.remove_prefix(1);
    while (!s.empty() && isWhitespace(s.back())) s.remove_suffix(1);
    return s;
}

inline ParseResult64 fromCharsUint64(std::string_view s) noexcept {
    s = trimView(s);
    if (s.empty()) return {0, false};

    bool negative = false;
    if (s.front() == '+') {
        s.remove_prefix(1);
        if (s.empty()) return {0, false};
    } else if (s.front() == '-') {
        negative = true;
        s.remove_prefix(1);
        if (s.empty()) return {0, false};
    }

    int base = 10;
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s.remove_prefix(2);
        if (s.empty()) return {0, false};
    }

    if (s.empty()) return {0, false};

    uint64_t result = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result, base);
    if (ec != std::errc{} || ptr != s.data() + s.size()) {
        return {0, false};
    }

    if (negative) return {0, false};
    return {result, true};
}

inline ParseResult32 fromCharsUint32(std::string_view s) noexcept {
    auto r = fromCharsUint64(s);
    if (!r.ok) return {0, false};
    if (r.value > UINT32_MAX) return {0, false};
    return {static_cast<uint32_t>(r.value), true};
}

inline ParseResult16 fromCharsUint16(std::string_view s, int base = 10) noexcept {
    s = trimView(s);
    if (s.empty()) return {0, false};

    uint16_t result = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result, base);
    if (ec != std::errc{} || ptr != s.data() + s.size()) {
        return {0, false};
    }
    return {result, true};
}

inline ParseResult64 parseVersionSegment(std::string_view s) noexcept {
    s = trimView(s);
    if (s.empty()) return {0, false};

    uint64_t result = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result, 10);
    if (ec != std::errc{} || ptr != s.data() + s.size()) {
        return {0, false};
    }
    return {result, true};
}

} // namespace mbootcore
