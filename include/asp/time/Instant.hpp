#pragma once

#include <asp/detail/config.hpp>
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

    Instant operator+(const Duration& dur) const;
    Instant operator-(const Duration& dur) const;
    Instant& operator+=(const Duration& dur);
    Instant& operator-=(const Duration& dur);

    i64 rawNanos() const;
    static Instant fromRawNanos(i64 nanos);

private:
    constexpr Instant(u64 s, u64 n) : m_secs(s), m_nanos(n) {}
    u64 m_secs  = 0;
    u64 m_nanos = 0;
};

}