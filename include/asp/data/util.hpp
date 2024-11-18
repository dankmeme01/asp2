#pragma once

#include <asp/misc/traits.hpp>
#include <asp/detail/config.hpp>
#include <string_view>
#include <string>

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

    inline void _constexprFail() {}
    inline void _constexprParseFailure() {}


    template <std::integral T>
    constexpr T constexprParse(std::string_view txt) {
        T result = 0;
        bool neg = false;
        size_t idx = 0;
        for (char c : txt) {
            if (c == '-') {
                if constexpr (!std::is_signed_v<T>) {
                    // unexpected character, invoke an error
                    if (std::is_constant_evaluated()) {
                        detail::assertionFail("expected unsigned type, but found negative sign");
                    } else {
                        _constexprParseFailure();
                    }
                }

                if (idx == 0) {
                    neg = true;
                    continue;
                } else {
                    if (std::is_constant_evaluated()) {
                        detail::assertionFail("unexpected negative sign in the middle of the number");
                    } else {
                        _constexprParseFailure();
                    }
                }
            }

            if (c < '0' || c > '9') {
                // unexpected character, invoke an error
                if (std::is_constant_evaluated()) {
                    detail::assertionFail("unexpected character in number");
                } else {
                    _constexprParseFailure();
                }
            }

            // check for overflow
            if (result > (std::numeric_limits<T>::max() - (c - '0')) / 10) {
                // overflow, invoke an error
                if (std::is_constant_evaluated()) {
                    detail::assertionFail("overflow in number");
                } else {
                    _constexprParseFailure();
                }
            }

            result = result * 10 + (c - '0');
            idx++;
        }

        return result;
    }

    template <std::integral T>
    constexpr std::string constexprToString(T val) {
        if (val == 0) {
            return "0";
        }

        std::string str;
        if constexpr (std::is_signed_v<T>) {
            if (val < 0) {
                str += '-';
                val = -val;
            }
        }

        T tmp = val;
        while (tmp > 0) {
            str += '0' + (tmp % 10);
            tmp /= 10;
        }

        std::reverse(str.begin(), str.end());
        return str;
    }
}


#undef BSWAP16
#undef BSWAP32
#undef BSWAP64