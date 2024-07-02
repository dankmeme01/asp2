#pragma once

namespace asp::data {
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
}
