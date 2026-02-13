#pragma once

#include <asp/detail/config.hpp>
#include <optional>
#include <string>
#include <array>
#include <fmt/format.h>

#include "detail.hpp"

namespace asp::inline time {

class Duration {
public:
    constexpr Duration() noexcept : m_seconds(0), m_nanos(0) {}
    constexpr Duration(const Duration& other) noexcept = default;
    constexpr Duration& operator=(const Duration& other) noexcept = default;
    constexpr Duration(Duration&& other) noexcept = default;
    constexpr Duration& operator=(Duration&& other) noexcept = default;

    constexpr Duration(u64 seconds, u32 nanos) noexcept : m_seconds(seconds) {
        if (nanos < time_detail::NANOS_IN_SEC) {
            m_nanos = nanos;
            return;
        }

        seconds += (nanos / time_detail::NANOS_IN_SEC);
        m_nanos = nanos % time_detail::NANOS_IN_SEC;
    }


    template <typename T>
    constexpr static auto fromSecs(T secs) noexcept {
        if constexpr (std::is_integral_v<T>) {
            return _unchecked(static_cast<u64>(secs), 0);
        } else if constexpr (std::is_floating_point_v<T>) {
            auto posZero = static_cast<T>(+0.0);
            if (secs < posZero) {
                return std::optional<Duration>{};
            }

            auto whole = time_detail::floor(secs);
            auto fractional = secs - (u64)secs;

            return std::optional{Duration(
                static_cast<u64>(whole),
                static_cast<u32>(static_cast<T>(time_detail::NANOS_IN_SEC) * fractional)
            )};
        } else {
            static_assert(!std::is_same_v<T, T>, "fromSecs only supports numbers");
        }
    }

    constexpr static std::optional<Duration> fromSecsF32(f32 secs) noexcept {
        return fromSecs(secs);
    }

    constexpr static std::optional<Duration> fromSecsF64(f64 secs) noexcept {
        return fromSecs(secs);
    }

    constexpr static Duration fromNanos(u64 nanos) noexcept {
        return _unchecked(time_detail::nanos_to_secs(nanos), nanos % time_detail::NANOS_IN_SEC);
    }
    constexpr static Duration fromMicros(u64 micros) noexcept {
        return _unchecked(time_detail::micros_to_secs(micros), time_detail::micros_to_nanos_subsec(micros));
    }
    constexpr static Duration fromMillis(u64 ms) noexcept {
        return _unchecked(time_detail::millis_to_secs(ms), time_detail::millis_to_nanos_subsec(ms));
    }
    constexpr static Duration fromMinutes(u64 mins) noexcept { return _unchecked(time_detail::minutes_to_secs(mins), 0); }
    constexpr static Duration fromHours(u64 hours) noexcept { return _unchecked(time_detail::hours_to_secs(hours), 0); }
    constexpr static Duration fromDays(u64 days) noexcept { return _unchecked(time_detail::days_to_secs(days), 0); }
    constexpr static Duration fromWeeks(u64 weeks) noexcept { return _unchecked(time_detail::weeks_to_secs(weeks), 0); }
    constexpr static Duration fromYears(u64 years) noexcept { return _unchecked(time_detail::years_to_secs(years), 0); }

    constexpr static Duration zero() noexcept {
        return Duration(0, 0);
    }

    constexpr static Duration infinite() noexcept {
        return Duration((u64)-1, time_detail::NANOS_IN_SEC - 1);
    }

    // Getter / converter functions

    constexpr bool isZero() const noexcept {
        return m_seconds == 0 && m_nanos == 0;
    }

    constexpr u64 micros() const noexcept {
        return static_cast<u64>(m_seconds) * static_cast<u64>(time_detail::MICROS_IN_SEC) + static_cast<u64>(time_detail::nanos_to_micros(m_nanos));
    }

    constexpr u64 nanos() const noexcept {
        return static_cast<u64>(m_seconds) * static_cast<u64>(time_detail::NANOS_IN_SEC) + static_cast<u64>(m_nanos);
    }

    template <typename As = u64>
    constexpr As millis() const noexcept;

    template <typename As = u64>
    constexpr As seconds() const noexcept;

    constexpr u64 minutes() const noexcept { return time_detail::secs_to_minutes(m_seconds); }
    constexpr u64 hours() const noexcept { return time_detail::secs_to_hours(m_seconds); }
    constexpr u64 days() const noexcept { return time_detail::secs_to_days(m_seconds); }
    constexpr u64 weeks() const noexcept { return time_detail::secs_to_weeks(m_seconds); }

    constexpr u64 subsecMillis() const noexcept { return time_detail::nanos_to_millis(m_nanos); }
    constexpr u64 subsecMicros() const noexcept { return time_detail::nanos_to_micros(m_nanos); }
    constexpr u64 subsecNanos() const noexcept { return m_nanos; }

    // Various operations

    [[nodiscard]] constexpr Duration absDiff(const Duration& other) const noexcept {
        if (auto res = this->checkedSub(other)) {
            return res.value();
        } else {
            return other.checkedSub(*this).value();
        }
    }

    [[nodiscard]] constexpr std::optional<Duration> checkedAdd(const Duration& other) const noexcept {
        u64 addedSecs;
        if (!::asp::checkedAdd(addedSecs, m_seconds, other.m_seconds)) {
            return std::nullopt;
        }

        u32 nanos = this->m_nanos + other.m_nanos;
        if (nanos > time_detail::NANOS_IN_SEC) {
            nanos -= time_detail::NANOS_IN_SEC;

            u64 temp;
            if (!::asp::checkedAdd(temp, addedSecs, 1ULL)) {
                return std::nullopt;
            }

            addedSecs = temp;
        }

        return _unchecked(addedSecs, nanos);
    }

    [[nodiscard]] constexpr std::optional<Duration> checkedSub(const Duration& other) const noexcept {
        u64 subdSecs;
        if (!::asp::checkedSub(subdSecs, m_seconds, other.m_seconds)) {
            return std::nullopt;
        }

        u32 nanos;
        if (m_nanos >= other.m_nanos) {
            nanos = m_nanos - other.m_nanos;
        } else {
            u64 temp;
            if (!::asp::checkedSub(temp, subdSecs, 1ULL)) {
                return std::nullopt;
            }

            subdSecs = temp;
            nanos = m_nanos + time_detail::NANOS_IN_SEC - other.m_nanos;
        }

        return _unchecked(subdSecs, nanos);
    }

    [[nodiscard]] constexpr std::optional<Duration> checkedMul(u32 by) const noexcept {
        auto totalNanos = static_cast<u64>(m_nanos) * static_cast<u64>(by);
        auto extraSecs = time_detail::nanos_to_secs(totalNanos);
        auto nanos = (totalNanos % time_detail::NANOS_IN_SEC);

        u64 mulSecs;
        if (!::asp::checkedMul(mulSecs, m_seconds, static_cast<u64>(by))) {
            return std::nullopt;
        }

        u64 addedSecs;
        if (!::asp::checkedAdd(addedSecs, mulSecs, extraSecs)) {
            return std::nullopt;
        }

        return _unchecked(addedSecs, nanos);
    }

    // Format duration to a string, for example 1.484ms, 3.141h, 1.234s
    std::string toString(u8 precision = 3) const;

    // Format duration to a human-readable string, for example 1.484 milliseconds, 3.141 hours, 1.234 seconds
    // Precision by default is 0, so only the integer part is formatted unless precision is specified
    std::string toHumanString(u8 precision = 0) const;

    // Operators

    ASP_CLANG_CONSTEXPR Duration operator+(const Duration& other) const {
        if (auto res = this->checkedAdd(other)) {
            return res.value();
        } else {
            time_detail::_throwrt("overflow when adding durations");
        }
    }

    ASP_CLANG_CONSTEXPR Duration& operator+=(const Duration& other) noexcept {
        *this = *this + other;
        return *this;
    }

    constexpr Duration operator-(const Duration& other) const noexcept {
        if (auto res = this->checkedSub(other)) {
            return res.value();
        } else {
            return Duration{};
        }
    }

    constexpr Duration& operator-=(const Duration& other) noexcept {
        *this = *this - other;
        return *this;
    }

    ASP_CLANG_CONSTEXPR Duration operator*(u32 val) const {
        if (auto res = this->checkedMul(val)) {
            return res.value();
        } else {
            time_detail::_throwrt("overflow when multiplying duration by scalar");
        }
    }

    ASP_CLANG_CONSTEXPR Duration& operator*=(u32 val) {
        *this = *this * val;
        return *this;
    }

    ASP_CLANG_CONSTEXPR Duration operator/(u32 val) const {
        if (val == 0) {
            time_detail::_throwrt("attempted to divide a Duration by 0");
        }

        u64 secq = m_seconds / val;
        u64 secr = m_seconds % val;

        u64 totalNanos = secr * time_detail::NANOS_IN_SEC + m_nanos;
        u64 nanosq = totalNanos / val;

        if (nanosq >= time_detail::NANOS_IN_SEC) {
            secq += nanosq / time_detail::NANOS_IN_SEC;
            nanosq %= time_detail::NANOS_IN_SEC;
        }

        return Duration(secq, nanosq);
    }

    ASP_CLANG_CONSTEXPR Duration& operator/=(u32 val) {
        *this = *this / val;
        return *this;
    }

    constexpr std::strong_ordering operator<=>(const Duration& other) const noexcept {
        if (m_seconds < other.m_seconds) {
            return std::strong_ordering::less;
        } else if (m_seconds > other.m_seconds) {
            return std::strong_ordering::greater;
        }

        if (m_nanos < other.m_nanos) {
            return std::strong_ordering::less;
        } else if (m_nanos > other.m_nanos) {
            return std::strong_ordering::greater;
        }

        return std::strong_ordering::equal;
    }

private:
    u64 m_seconds;
    // Invariant: m_nanos always between 0 and `s_to_ns(1)`, so less than a second
    u32 m_nanos;

    constexpr static Duration _unchecked(u64 secs, u32 nanos) noexcept {
        Duration dur;
        dur.m_seconds = secs;
        dur.m_nanos = nanos;
        return dur;
    }
};


template<>
constexpr u64 Duration::millis<u64>() const noexcept {
    return m_seconds * time_detail::MILLIS_IN_SEC + time_detail::nanos_to_millis(m_nanos);
}

template<>
constexpr f32 Duration::millis<f32>() const noexcept {
    return (f32)m_seconds * (f32)time_detail::MILLIS_IN_SEC + (f32)m_nanos / (f32)time_detail::NANOS_IN_MILLISEC;
}

template<>
constexpr f64 Duration::millis<f64>() const noexcept {
    return (f64)m_seconds * (f64)time_detail::MILLIS_IN_SEC + (f64)m_nanos / (f64)time_detail::NANOS_IN_MILLISEC;
}

template<>
constexpr u64 Duration::seconds<u64>() const noexcept {
    return m_seconds;
}

template<>
constexpr f32 Duration::seconds<f32>() const noexcept {
    return static_cast<f32>(m_seconds) + static_cast<f32>(m_nanos) / static_cast<f32>(time_detail::NANOS_IN_SEC);
}

template<>
constexpr f64 Duration::seconds<f64>() const noexcept {
    return static_cast<f64>(m_seconds) + static_cast<f64>(m_nanos) / static_cast<f64>(time_detail::NANOS_IN_SEC);
}

ASP_CLANG_CONSTEXPR Duration operator*(u32 val, const Duration& dur) {
    return dur * val;
}

// for some reason this is necessary ?? the spaceship operator just doesn't work

constexpr inline bool operator==(const Duration& lhs, const Duration& rhs) noexcept {
    return lhs.seconds() == rhs.seconds() && lhs.subsecNanos() == rhs.subsecNanos();
}

constexpr inline bool operator!=(const Duration& lhs, const Duration& rhs) noexcept {
    return !(lhs == rhs);
}

enum class DurationUnit {
    Nanos,
    Micros,
    Millis,
    Secs,
    Mins,
    Hours,
};

std::string_view suffixForUnit(DurationUnit unit, bool human) noexcept;

std::pair<std::string_view, DurationUnit> formatDurationNum(
    std::array<char, 32>& buf, const asp::Duration& dur, int precision
);

} // namespace asp::time


template <>
struct fmt::formatter<asp::Duration> {
    int precision = 3;

    auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && *it == '.') {
            ++it;
            precision = 0;
            while (it != end && *it >= '0' && *it <= '9') {
                precision = precision * 10 + (*it - '0');
                ++it;
            }
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const asp::time::Duration& dur, FormatContext& ctx) const -> FormatContext::iterator {
        std::array<char, 32> buf;
        auto [num, unit] = asp::formatDurationNum(buf, dur, precision);
        return fmt::format_to(ctx.out(), "{}{}", num, asp::suffixForUnit(unit, false));
    }
};
