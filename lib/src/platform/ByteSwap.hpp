#pragma once

#include <cstdint>

#ifdef _MSC_VER
#include <stdlib.h>
#endif

namespace mbootcore {
namespace platform {

inline constexpr uint16_t byteswap(uint16_t value) noexcept {
#ifdef _MSC_VER
    return _byteswap_ushort(value);
#else
    return __builtin_bswap16(value);
#endif
}

inline constexpr uint32_t byteswap(uint32_t value) noexcept {
#ifdef _MSC_VER
    return _byteswap_ulong(value);
#else
    return __builtin_bswap32(value);
#endif
}

inline constexpr uint64_t byteswap(uint64_t value) noexcept {
#ifdef _MSC_VER
    return _byteswap_uint64(value);
#else
    return __builtin_bswap64(value);
#endif
}

} // namespace platform
} // namespace mbootcore
