#pragma once

#include <asp/data/nums.hpp>
#ifndef __clang__
# include <cmath> // std::floor
#endif

namespace asp::time_detail {
    constexpr inline u64 NANOS_IN_SEC = 1'000'000'000;
    constexpr inline u64 NANOS_IN_MILLISEC = 1'000'000;
    constexpr inline u64 NANOS_IN_MICROSEC = 1'000;
    constexpr inline u64 MILLIS_IN_SEC = 1'000;
    constexpr inline u64 MICROS_IN_SEC = 1'000'000;
    constexpr inline u64 SECS_IN_MIN = 60;
    constexpr inline u64 MINS_IN_HOUR = 60;
    constexpr inline u64 HOURS_IN_DAY = 24;
    constexpr inline u64 DAYS_IN_WEEK = 7;

    constexpr inline u64 SECS_IN_HOUR = SECS_IN_MIN * MINS_IN_HOUR;
    constexpr inline u64 SECS_IN_DAY = SECS_IN_HOUR * HOURS_IN_DAY;

    constexpr inline u64 minutes_to_secs(u64 in) { return in * SECS_IN_MIN; }
    constexpr inline u64 hours_to_secs(u64 in) { return minutes_to_secs(in * MINS_IN_HOUR); }
    constexpr inline u64 days_to_secs(u64 in) { return hours_to_secs(in * HOURS_IN_DAY); }
    constexpr inline u64 weeks_to_secs(u64 in) { return days_to_secs(in * DAYS_IN_WEEK); }
    constexpr inline u64 years_to_secs(u64 in) { return days_to_secs(in * 365); }

    constexpr inline u64 secs_to_minutes(u64 in) { return in / SECS_IN_MIN; }
    constexpr inline u64 secs_to_hours(u64 in) { return secs_to_minutes(in) / MINS_IN_HOUR; }
    constexpr inline u64 secs_to_days(u64 in) { return secs_to_hours(in) / HOURS_IN_DAY; }
    constexpr inline u64 secs_to_weeks(u64 in) { return secs_to_days(in) / DAYS_IN_WEEK; }

    constexpr inline u64 secs_to_millis(u64 in) { return in * MILLIS_IN_SEC; }
    constexpr inline u64 secs_to_micros(u64 in) { return in * MICROS_IN_SEC; }
    constexpr inline u64 secs_to_nanos(u64 in) { return in * NANOS_IN_SEC; }

    constexpr inline u64 millis_to_secs(u64 in) { return in / MILLIS_IN_SEC; }
    constexpr inline u64 millis_to_micros(u64 in) { return in * 1'000; }
    constexpr inline u64 millis_to_nanos(u64 in) { return in * 1'000'000; }
    constexpr inline u64 millis_to_nanos_subsec(u64 in) { return (in % MILLIS_IN_SEC) * 1'000'000; }

    constexpr inline u64 micros_to_secs(u64 in) { return in / MICROS_IN_SEC; }
    constexpr inline u64 micros_to_nanos(u64 in) { return in * 1'000; }
    constexpr inline u64 micros_to_nanos_subsec(u64 in) { return (in % MICROS_IN_SEC) * 1'000; }

    constexpr inline u64 nanos_to_secs(u64 in) { return in / NANOS_IN_SEC; }
    constexpr inline u64 nanos_to_millis(u64 in) { return in / NANOS_IN_MILLISEC; }
    constexpr inline u64 nanos_to_micros(u64 in) { return in / NANOS_IN_MICROSEC; }

    [[noreturn]] void _throwrt(const char* msg);

    template <typename T> T floor(T x);

#ifdef __clang__
    template<> inline f32 floor(f32 x) { return __builtin_floorf(x); }
    template<> inline f64 floor(f64 x) { return __builtin_floor(x); }
#else
    template<> inline f32 floor(f32 x) { return std::floor(x); }
    template<> inline f64 floor(f64 x) { return std::floor(x); }
#endif
}