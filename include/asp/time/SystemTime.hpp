#pragma once

#include "detail.hpp"
#include "DateTime.hpp"
#include "Duration.hpp"
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <optional>

#include <time.h>

namespace asp::inline time {
    inline std::tm localtime(std::time_t time) {
        struct std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        return tm;
    }

    class SystemTime {
    public:
        constexpr SystemTime(const SystemTime& other) = default;
        constexpr SystemTime& operator=(const SystemTime& other) = default;
        constexpr SystemTime(SystemTime&& other) = default;
        constexpr SystemTime& operator=(SystemTime&& other) = default;

        // Default constructor initializes the SystemTime to the UNIX epoch.
        SystemTime();

        static SystemTime now();

        inline static SystemTime fromUnix(time_t t) {
            return UNIX_EPOCH + Duration::fromSecs(t);
        }

        inline static SystemTime fromUnixMillis(u64 ms) {
            return UNIX_EPOCH + Duration::fromMillis(ms);
        }

        static SystemTime UNIX_EPOCH;

        std::optional<Duration> durationSince(const SystemTime& other) const;

        inline Duration timeSinceEpoch() const {
            // we assume that this operation is infallible
            return this->durationSince(UNIX_EPOCH).value();
        }

        // Return the amount of time passed since this measurement was taken until now, or a zero duration.
        inline Duration elapsed() const {
            return SystemTime::now().durationSince(*this).value_or(Duration{});
        }

        // Return the amount of time until this measurement is reached, or a zero duration.
        inline Duration until() const {
            return this->durationSince(SystemTime::now()).value_or(Duration{});
        }

        inline bool isFuture() const {
            return SystemTime::now() < *this;
        }

        inline bool isPast() const {
            return *this < SystemTime::now();
        }

        time_t to_time_t() const;

        Date dateUtc() const;
        Time timeUtc() const;
        DateTime dateTimeUtc() const;

        inline std::string format(std::string_view fmt) const {
            time_t curTime = this->to_time_t();
            auto ms = this->timeSinceEpoch().subsecMillis();
            return fmt::format(fmt::runtime(fmt), asp::localtime(curTime), ms);
        }

        inline std::string toString(bool millis = false) const {
            return this->format(millis ? "{:%Y-%m-%d %H:%M:%S}.{:03}" : "{:%Y-%m-%d %H:%M:%S}");
        }


        std::optional<Duration> operator-(const SystemTime& other) const {
            return this->durationSince(other);
        }

        SystemTime operator+(const Duration& dur) const;
        std::strong_ordering operator<=>(const SystemTime& other) const;

    private:
        constexpr SystemTime(i64 _s, i64 _s2) : _storage1(_s), _storage2(_s2) {}

        static SystemTime _epoch();

        i64 _storage1;
        i64 _storage2;
    };
}
