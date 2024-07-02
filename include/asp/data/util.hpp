#pragma once

#include <asp/misc/traits.hpp>

#include <stdint.h>

#if defined(_MSC_VER) && !defined(__clang__)
# include <stdlib.h>
# define BSWAP16(val) _byteswap_ushort(val)
# define BSWAP32(val) _byteswap_ulong(val)
# define BSWAP64(val) _byteswap_uint64(val)
#else
# define BSWAP16(val) (uint16_t)((val >> 8) | (val << 8))
# define BSWAP32(val) __builtin_bswap32(val)
# define BSWAP64(val) __builtin_bswap64(val)
#endif

namespace asp::data {
    template <typename T>
    concept is_primitive = asp::is_one_of<T, bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double>;

#if defined(__clang__) && __has_builtin(__builtin_memcpy)
    # define DO_MEMCPY __builtin_memcpy
#else
    # define DO_MEMCPY _rmemcpy
    inline void _rmemcpy(void* dest, const void* src, size_t size) {
        char* d = (char*)dest;
        const char* s = (const char*)src;
        while (size--) {
            *d++ = *s++;
        }
    }
#endif

    template <typename To, typename From>
    static inline To bit_cast(From value) noexcept {
        static_assert(sizeof(From) == sizeof(To), "arguments for bit_cast must be of the same size");

        To dest;
        DO_MEMCPY(&dest, &value, sizeof(To));
        return dest;
    }

    // byteswap

    template <typename T>
    inline T byteswap(T val) {
        static_assert(is_primitive<T>, "Unsupported type for byteswap");

        if constexpr (std::is_same_v<T, uint16_t>) {
            return BSWAP16(val);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return BSWAP32(val);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return BSWAP64(val);
        } else if constexpr (std::is_same_v<T, float>) {
            return bit_cast<float>(BSWAP32(bit_cast<uint32_t>(val)));
        } else if constexpr (std::is_same_v<T, double>) {
            return bit_cast<double>(BSWAP64(bit_cast<uint64_t>(val)));
        } else if constexpr (std::is_same_v<T, int16_t>) {
            auto v = bit_cast<uint16_t>(val);
            return bit_cast<int16_t>(BSWAP16(v));
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return bit_cast<int32_t>(BSWAP32(bit_cast<uint32_t>(val)));
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return bit_cast<int64_t>(BSWAP64(bit_cast<uint64_t>(val)));
        } else if constexpr (asp::is_one_of<T, int8_t, uint8_t, bool>) {
            return val;
        }
    }
}


#undef BSWAP16
#undef BSWAP32
#undef BSWAP64