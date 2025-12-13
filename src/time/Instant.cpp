#include <asp/time/Instant.hpp>
#include <asp/time/detail.hpp>

#ifdef ASP_IS_WIN
# include <Windows.h>
#else
# include <time.h>
#endif

namespace asp::time {

#ifdef ASP_IS_WIN

static LARGE_INTEGER getFrequency() {
    static auto freq = []() -> LARGE_INTEGER {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();

    return freq;
}

static i64 qpc() {
    LARGE_INTEGER tx;
    QueryPerformanceCounter(&tx);
    return tx.QuadPart;
}

Instant Instant::now() {
    static auto epoch = qpc();
    LARGE_INTEGER freq = getFrequency();

    i64 elapsed = std::max<i64>(0, qpc() - epoch);

    // convert dt to seconds and nanos
    u64 secs = elapsed / freq.QuadPart;
    u64 remTicks = elapsed % freq.QuadPart;
    u64 nanos = (remTicks * 1'000'000'000ULL) / freq.QuadPart;

    return Instant{secs, nanos};
}

#else

Instant Instant::now() {
    timespec tp;
    if (0 != clock_gettime(CLOCK_MONOTONIC_RAW, &tp)) [[unlikely]] {
        detail::_throwrt("failed to get the current time");
    }

    return Instant{(u64)tp.tv_sec, (u64)tp.tv_nsec};
}

#endif

Instant Instant::farFuture() {
    return Instant{u64(1ull << 48), 0};
}

Duration Instant::durationSince(const Instant& other) const {
    i64 secs = m_secs - other.m_secs;
    i64 nanos = m_nanos - other.m_nanos;

    if (nanos < 0) {
        nanos += detail::NANOS_IN_SEC;
        secs -= 1;
    }

    // clamp to 0 if negative
    if (secs < 0) {
        secs = 0;
        nanos = 0;
    }

    return Duration{static_cast<u64>(secs), static_cast<u32>(nanos)};
}

i64 Instant::rawNanos() const {
    return (i64)(m_secs * detail::NANOS_IN_SEC + m_nanos);
}

Instant Instant::fromRawNanos(i64 nanos) {
    u64 secs = (u64)nanos / detail::NANOS_IN_SEC;
    u64 subsecNanos = (u64)nanos % detail::NANOS_IN_SEC;
    return Instant{secs, subsecNanos};
}

Instant Instant::operator+(const Duration& dur) const {
    auto secs = m_secs + dur.seconds();
    auto nanos = m_nanos + dur.subsecNanos();

    if (nanos >= detail::NANOS_IN_SEC) {
        nanos -= detail::NANOS_IN_SEC;
        secs += 1;
    }

    return Instant{secs, nanos};
}

Instant Instant::operator-(const Duration& dur) const {
    if (dur.seconds() > m_secs) {
        return Instant{0, 0};
    }

    auto secs = m_secs - dur.seconds();
    auto nanos = (i64)m_nanos - (i64)dur.subsecNanos();

    if (nanos < 0) {
        if (secs == 0) {
            return Instant{0, 0};
        }

        nanos += detail::NANOS_IN_SEC;
        secs -= 1;
    }

    return Instant{secs, (u64)nanos};
}

Instant& Instant::operator+=(const Duration& dur) {
    *this = *this + dur;
    return *this;
}

Instant& Instant::operator-=(const Duration& dur) {
    *this = *this - dur;
    return *this;
}

std::strong_ordering Instant::operator<=>(const Instant& other) const {
    auto scmp = m_secs <=> other.m_secs;
    auto ncmp = m_nanos <=> other.m_nanos;

    return (scmp != std::strong_ordering::equal) ? scmp : ncmp;
}

}