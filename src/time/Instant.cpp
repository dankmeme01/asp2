#include <asp/time/Instant.hpp>
#include <asp/time/detail.hpp>
#include <asp/data/nums.hpp>
#include <stdexcept>
#include <cstdlib>

#ifdef ASP_IS_WIN
# include <Windows.h>
#else
# include <time.h>
#endif

namespace asp::inline time {

#ifdef ASP_IS_WIN

static LARGE_INTEGER getFrequency() noexcept {
    static auto freq = []() -> LARGE_INTEGER {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();

    return freq;
}

static i64 qpc() noexcept {
    LARGE_INTEGER tx;
    QueryPerformanceCounter(&tx);
    return tx.QuadPart;
}

Instant Instant::now() noexcept {
    LARGE_INTEGER freq = getFrequency();

    u64 elapsed = qpc();

    // convert dt to seconds and nanos
    u64 secs = elapsed / freq.QuadPart;
    u64 remTicks = elapsed % freq.QuadPart;
    u64 nanos = (remTicks * 1'000'000'000ULL) / freq.QuadPart;

    return Instant{secs, nanos};
}

#else

Instant Instant::now() noexcept {
    timespec tp;
    int rc = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);

    if (rc != 0) [[unlikely]] std::abort();

    return Instant{(u64)tp.tv_sec, (u64)tp.tv_nsec};
}

#endif

Instant Instant::farFuture() noexcept {
    return Instant{u64(1ull << 48), 0};
}

Duration Instant::durationSince(const Instant& other) const noexcept {
    i64 secs = m_secs - other.m_secs;
    i64 nanos = m_nanos - other.m_nanos;

    if (nanos < 0) {
        nanos += time_detail::NANOS_IN_SEC;
        secs -= 1;
    }

    // clamp to 0 if negative
    if (secs < 0) {
        secs = 0;
        nanos = 0;
    }

    return Duration{static_cast<u64>(secs), static_cast<u32>(nanos)};
}

i64 Instant::rawNanos() const noexcept {
    return (i64)(m_secs * time_detail::NANOS_IN_SEC + m_nanos);
}

Instant Instant::fromRawNanos(i64 nanos) noexcept {
    u64 secs = (u64)nanos / time_detail::NANOS_IN_SEC;
    u64 subsecNanos = (u64)nanos % time_detail::NANOS_IN_SEC;
    return Instant{secs, subsecNanos};
}

std::optional<Instant> Instant::checkedAdd(const Duration& dur) const noexcept {
    u64 added;
    if (!asp::checkedAdd(added, m_secs, dur.seconds())) {
        return std::nullopt;
    }

    u64 extra = 0;
    u32 nanos = m_nanos + dur.subsecNanos();
    if (nanos >= time_detail::NANOS_IN_SEC) {
        nanos -= time_detail::NANOS_IN_SEC;
        extra = 1;
    }

    if (!asp::checkedAdd(added, added, extra)) {
        return std::nullopt;
    }

    return Instant{added, nanos};
}

std::optional<Instant> Instant::checkedSub(const Duration& dur) const noexcept {
    u64 subbed;
    if (!asp::checkedSub(subbed, m_secs, dur.seconds())) {
        return std::nullopt;
    }

    u32 nanos;
    u32 durNanos = dur.subsecNanos();
    if (m_nanos > durNanos) {
        nanos = m_nanos - durNanos;
    } else {
        u64 temp;
        if (!asp::checkedSub(temp, subbed, 1ULL)) {
            return std::nullopt;
        }

        subbed = temp;
        nanos = m_nanos + time_detail::NANOS_IN_SEC - durNanos;
    }

    return Instant{subbed, (u64)nanos};
}

Duration Instant::absDiff(const Instant& other) const noexcept {
    if (*this >= other) {
        return this->durationSince(other);
    } else {
        return other.durationSince(*this);
    }
}

Instant Instant::saturatingAdd(const Duration& dur) const noexcept {
    return this->checkedAdd(dur).value_or(Instant::farFuture());
}

Instant Instant::saturatingSub(const Duration& dur) const noexcept {
    return this->checkedSub(dur).value_or(Instant{});
}

Instant Instant::operator+(const Duration& dur) const noexcept {
    return this->saturatingAdd(dur);
}

Instant Instant::operator-(const Duration& dur) const noexcept {
    return this->saturatingSub(dur);
}

Instant& Instant::operator+=(const Duration& dur) noexcept {
    *this = *this + dur;
    return *this;
}

Instant& Instant::operator-=(const Duration& dur) noexcept {
    *this = *this - dur;
    return *this;
}

std::strong_ordering Instant::operator<=>(const Instant& other) const noexcept {
    auto scmp = m_secs <=> other.m_secs;
    auto ncmp = m_nanos <=> other.m_nanos;

    return (scmp != std::strong_ordering::equal) ? scmp : ncmp;
}

}