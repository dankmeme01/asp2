#pragma once

#ifndef __clang__
# include <limits>
#endif

namespace asp::inline nums {
    using u8 = unsigned char;
    using i8 = signed char;

    using u16 = unsigned short;
    using i16 = signed short;

    using u32 = unsigned int;
    using i32 = signed int;

    using u64 = unsigned long long;
    using i64 = signed long long;

    using f32 = float;
    using f64 = double;

#if ((defined __GNUC__ || defined __clang__) && defined ASP_USE_INT128)
    using i128 = __int128;
    using u128 = unsigned __int128;
#else
    using i128 = i64;
    using u128 = u64;
#endif

    // If an overflow occurred, returns 'false' and contents of `out` are undefined. Otherwise, returns `true` and `out = a + b`
    template <typename T>
    constexpr bool checkedAdd(T& out, T a, T b) {
#ifdef __clang__
        return __builtin_add_overflow(a, b, &out) == 0;
#else
        if (b > std::numeric_limits<T>::max() - a) {
            return false;
        }

        out = a + b;
        return true;
#endif
    }

    // If an overflow occurred, returns 'false' and contents of `out` are undefined. Otherwise, returns `true` and `out = a - b`
    template <typename T>
    constexpr bool checkedSub(T& out, T a, T b) {
#ifdef __clang__
        return __builtin_sub_overflow(a, b, &out) == 0;
#else
        if (b > a) {
            return false;
        }

        out = a - b;
        return true;
#endif
    }

    // If an overflow occurred, returns 'false' and contents of `out` are undefined. Otherwise, returns `true` and `out = a * b`
    template <typename T>
    constexpr bool checkedMul(T& out, T a, T b) {
#ifdef __clang__
        return __builtin_mul_overflow(a, b, &out) == 0;
#else
        if (a != 0 && b > std::numeric_limits<T>::max() / a) {
            return false;
        }

        out = a * b;
        return true;
#endif
    }
}
