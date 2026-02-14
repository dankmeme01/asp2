#pragma once

#include <asp/detail/config.hpp>
#include "Duration.hpp"

namespace asp::inline time {

class Instant {
public:
    constexpr Instant(const Instant& other) noexcept = default;
    constexpr Instant& operator=(const Instant& other) noexcept = default;
    constexpr Instant(Instant&& other) noexcept = default;
    constexpr Instant& operator=(Instant&& other) noexcept = default;

    static Instant now() noexcept;

    /// Returns an instant representing the far future, likely never to be reached.
    static Instant farFuture() noexcept;

    /// Indeterminate state, but guaranteed to be earlier than any valid instant returned by `now()`.
    constexpr Instant() noexcept {}

    /// Returns how much time has passed since `other` until this instant.
    /// If `other` is in the future, the duration will be zero.
    Duration durationSince(const Instant& other) const noexcept;

    /// Returns how much time has passed since this instant.
    /// If the instant is in the future, the duration will be zero.
    inline Duration elapsed() const noexcept {
        return Instant::now().durationSince(*this);
    }

    /// Returns how much time is left until this instant.
    /// If the instant is in the past, the duration will be zero.
    inline Duration until() const noexcept {
        return this->durationSince(Instant::now());
    }

    /// Adds the given duration to this instant, returning std::nullopt on overflow.
    std::optional<Instant> checkedAdd(const Duration& dur) const noexcept;
    /// Subtracts the given duration from this instant, returning std::nullopt on overflow.
    std::optional<Instant> checkedSub(const Duration& dur) const noexcept;
    /// Returns the absolute difference between this instant and another instant.
    Duration absDiff(const Instant& other) const noexcept;

    /// Adds the given duration to this instant, returning `farFuture()` on overflow.
    Instant saturatingAdd(const Duration& dur) const noexcept;
    /// Subtracts the given duration from this instant, returning `Instant()` on overflow.
    Instant saturatingSub(const Duration& dur) const noexcept;

    /// Shorthand for `saturatingAdd()`, adds the given duration to this instant, returning `farFuture()` on overflow.
    Instant operator+(const Duration& dur) const noexcept;
    /// Shorthand for `saturatingSub()`, subtracts the given duration from this instant, returning `Instant()` on overflow.
    Instant operator-(const Duration& dur) const noexcept;
    Instant& operator+=(const Duration& dur) noexcept;
    Instant& operator-=(const Duration& dur) noexcept;

    std::strong_ordering operator<=>(const Instant& other) const noexcept;
    bool operator==(const Instant& other) const noexcept = default;

    i64 rawNanos() const noexcept;
    static Instant fromRawNanos(i64 nanos) noexcept;

private:
    constexpr Instant(u64 s, u64 n) : m_secs(s), m_nanos(n) {}
    u64 m_secs  = 0;
    u64 m_nanos = 0;
};

}