#pragma once

#include "detail.hpp"
#include "DateTime.hpp"
#include "Duration.hpp"
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <optional>

#include <time.h>

namespace asp::inline time {
    inline std::tm localtime(std::time_t time) noexcept {
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        return tm;
    }

    class SystemTime {
    public:
        constexpr SystemTime(const SystemTime& other) noexcept = default;
        constexpr SystemTime& operator=(const SystemTime& other) noexcept = default;
        constexpr SystemTime(SystemTime&& other) noexcept = default;
        constexpr SystemTime& operator=(SystemTime&& other) noexcept = default;

        // Default constructor initializes the SystemTime to the UNIX epoch.
        SystemTime() noexcept;

        static SystemTime now() noexcept;

        static SystemTime fromUnix(time_t t) noexcept {
            return UNIX_EPOCH + Duration::fromSecs(t);
        }

        static SystemTime fromUnixMillis(u64 ms) noexcept {
            return UNIX_EPOCH + Duration::fromMillis(ms);
        }

        static SystemTime UNIX_EPOCH;

        std::optional<Duration> durationSince(const SystemTime& other) const noexcept;

        inline Duration timeSinceEpoch() const noexcept {
            // we assume that this operation is infallible
            return this->durationSince(UNIX_EPOCH).value();
        }

        // Return the amount of time passed since this measurement was taken until now, or a zero duration.
        inline Duration elapsed() const noexcept {
            return SystemTime::now().durationSince(*this).value_or(Duration{});
        }

        // Return the amount of time until this measurement is reached, or a zero duration.
        inline Duration until() const noexcept {
            return this->durationSince(SystemTime::now()).value_or(Duration{});
        }

        inline bool isFuture() const noexcept {
            return SystemTime::now() < *this;
        }

        inline bool isPast() const noexcept {
            return *this < SystemTime::now();
        }

        time_t to_time_t() const noexcept;

        Date dateUtc() const noexcept;
        Time timeUtc() const noexcept;
        DateTime dateTimeUtc() const noexcept;

        inline std::string format(std::string_view fmt) const {
            time_t curTime = this->to_time_t();
            auto ms = this->timeSinceEpoch().subsecMillis();
            return fmt::format(fmt::runtime(fmt), asp::localtime(curTime), ms);
        }

        inline std::string toString(bool millis = false) const {
            return this->format(millis ? "{:%Y-%m-%d %H:%M:%S}.{:03}" : "{:%Y-%m-%d %H:%M:%S}");
        }

        std::optional<Duration> operator-(const SystemTime& other) const noexcept {
            return this->durationSince(other);
        }

        SystemTime operator+(const Duration& dur) const noexcept;
        std::strong_ordering operator<=>(const SystemTime& other) const noexcept;

    private:
        constexpr SystemTime(i64 _s, i64 _s2) noexcept : _storage1(_s), _storage2(_s2) {}

        static SystemTime _epoch() noexcept;

        i64 _storage1;
        i64 _storage2;
    };
}
