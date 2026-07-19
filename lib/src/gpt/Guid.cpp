#include "mbootcore/gpt/Guid.hpp"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>

namespace mbootcore {
namespace gpt {

const Guid Guid::Null{};

Guid Guid::fromString(std::string_view s) {
    Guid g{};
    auto trimmed = s;
    while (!trimmed.empty() && (trimmed.front() == '{' || trimmed.front() == '('))
        trimmed.remove_prefix(1);
    while (!trimmed.empty() && (trimmed.back() == '}' || trimmed.back() == ')'))
        trimmed.remove_suffix(1);
    if (trimmed.size() < 36) return g;
    unsigned int d1, d2, d3;
    unsigned int d4a, d4b, d4c, d4d, d4e, d4f, d4g, d4h;
    int n = std::sscanf(trimmed.data(),
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &d1, &d2, &d3,
        &d4a, &d4b, &d4c, &d4d, &d4e, &d4f, &d4g, &d4h);
    if (n == 11) {
        g.data1 = static_cast<uint32_t>(d1);
        g.data2 = static_cast<uint16_t>(d2);
        g.data3 = static_cast<uint16_t>(d3);
        g.data4 = {
            static_cast<uint8_t>(d4a), static_cast<uint8_t>(d4b),
            static_cast<uint8_t>(d4c), static_cast<uint8_t>(d4d),
            static_cast<uint8_t>(d4e), static_cast<uint8_t>(d4f),
            static_cast<uint8_t>(d4g), static_cast<uint8_t>(d4h)
        };
    }
    return g;
}

std::string Guid::toString() const {
    char buf[37];
    std::snprintf(buf, sizeof(buf),
        "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        data1, data2, data3,
        data4[0], data4[1], data4[2], data4[3],
        data4[4], data4[5], data4[6], data4[7]);
    return std::string(buf, 36);
}

bool Guid::isNull() const noexcept {
    return data1 == 0 && data2 == 0 && data3 == 0 &&
           data4[0] == 0 && data4[1] == 0 && data4[2] == 0 && data4[3] == 0 &&
           data4[4] == 0 && data4[5] == 0 && data4[6] == 0 && data4[7] == 0;
}

bool Guid::operator==(const Guid& other) const noexcept {
    return data1 == other.data1 && data2 == other.data2 && data3 == other.data3 &&
           data4[0] == other.data4[0] && data4[1] == other.data4[1] &&
           data4[2] == other.data4[2] && data4[3] == other.data4[3] &&
           data4[4] == other.data4[4] && data4[5] == other.data4[5] &&
           data4[6] == other.data4[6] && data4[7] == other.data4[7];
}

bool Guid::operator!=(const Guid& other) const noexcept {
    return !(*this == other);
}

}} // namespace mbootcore::gpt
