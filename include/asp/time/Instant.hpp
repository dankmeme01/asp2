#pragma once

#include <asp/detail/config.hpp>
#include "Duration.hpp"

namespace asp::inline time {

class Instant {
public:
    constexpr Instant(const Instant& other) = default;
    constexpr Instant& operator=(const Instant& other) = default;
    constexpr Instant(Instant&& other) = default;
    constexpr Instant& operator=(Instant&& other) = default;

    static Instant now();

    /// Returns an instant representing the far future, likely never to be reached.
    static Instant farFuture();

    // Undeterminate state, do not use
    constexpr inline Instant() {}

    Duration durationSince(const Instant& other) const;

    inline Duration elapsed() const {
        return Instant::now().durationSince(*this);
    }

    /// Adds the given duration to this instant, returning std::nullopt on overflow.
    std::optional<Instant> checkedAdd(const Duration& dur) const;
    /// Subtracts the given duration from this instant, returning std::nullopt on overflow.
    std::optional<Instant> checkedSub(const Duration& dur) const;
    /// Returns the absolute difference between this instant and another instant.
    Duration absDiff(const Instant& other) const;

    /// Adds the given duration to this instant, throwing on overflow.
    Instant operator+(const Duration& dur) const;
    /// Subtracts the given duration from this instant, throwing on overflow.
    Instant operator-(const Duration& dur) const;
    Instant& operator+=(const Duration& dur);
    Instant& operator-=(const Duration& dur);

    std::strong_ordering operator<=>(const Instant& other) const;
    bool operator==(const Instant& other) const = default;

    i64 rawNanos() const;
    static Instant fromRawNanos(i64 nanos);

private:
    constexpr Instant(u64 s, u64 n) : m_secs(s), m_nanos(n) {}
    u64 m_secs  = 0;
    u64 m_nanos = 0;
};

}