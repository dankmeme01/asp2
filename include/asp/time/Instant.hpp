#pragma once

#include <asp/detail/config.hpp>
#include <optional>
#include "detail.hpp"
#include "Duration.hpp"

namespace asp::time {
    class Instant {
    public:
        constexpr Instant(const Instant& other) = default;
        constexpr Instant& operator=(const Instant& other) = default;
        constexpr Instant(Instant&& other) = default;
        constexpr Instant& operator=(Instant&& other) = default;

        static Instant now();

        // Undeterminate state, do not use
        constexpr inline Instant() {}

        Duration durationSince(const Instant& other) const;

        inline Duration elapsed() const {
            return Instant::now().durationSince(*this);
        }

    private:
#ifdef ASP_IS_WIN
        constexpr Instant(i64 _s) : _storage(_s) {}
        i64 _storage;
#else
        constexpr Instant(i64 _s, i64 _s2) : _storage1(_s), _storage2(_s2) {}
        i64 _storage1;
        i64 _storage2;
#endif
    };
}