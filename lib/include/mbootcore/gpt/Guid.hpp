#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <array>
#include <functional>

namespace mbootcore {
namespace gpt {

struct Guid {
    uint32_t data1{0};
    uint16_t data2{0};
    uint16_t data3{0};
    std::array<uint8_t, 8> data4{};

    static Guid fromString(std::string_view s);
    std::string toString() const;

    bool isNull() const noexcept;

    bool operator==(const Guid& other) const noexcept;
    bool operator!=(const Guid& other) const noexcept;

    static const Guid Null;
};

}} // namespace mbootcore::gpt

namespace std {
template<>
struct hash<mbootcore::gpt::Guid> {
    size_t operator()(const mbootcore::gpt::Guid& g) const noexcept {
        size_t h = 0;
        h ^= std::hash<uint32_t>{}(g.data1) + 0x9E3779B9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint16_t>{}(g.data2) + 0x9E3779B9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint16_t>{}(g.data3) + 0x9E3779B9 + (h << 6) + (h >> 2);
        for (auto b : g.data4) {
            h ^= std::hash<uint8_t>{}(b) + 0x9E3779B9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};
}
