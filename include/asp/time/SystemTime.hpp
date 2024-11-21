#pragma once

#include "detail.hpp"
#include <optional>
#include "Duration.hpp"

#include <time.h>
#include <limits>

namespace asp::time {
    class SystemTime {
        constexpr static i64 INVALID_VALUE = std::numeric_limits<i64>::min();

    public:
        constexpr SystemTime(const SystemTime& other) = default;
        constexpr SystemTime& operator=(const SystemTime& other) = default;
        constexpr SystemTime(SystemTime&& other) = default;
        constexpr SystemTime& operator=(SystemTime&& other) = default;

        constexpr SystemTime() : SystemTime(INVALID_VALUE, INVALID_VALUE) {}

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
            this->_check_not_zero();
            // we assume that this operation is infallible
            return this->durationSince(UNIX_EPOCH).value();
        }

        // Return the amount of time passed since this measurement was taken until now.
        inline Duration elapsed() const {
            this->_check_not_zero();
            return SystemTime::now().durationSince(*this).value_or(Duration{});
        }

        inline bool isFuture() const {
            this->_check_not_zero();
            return SystemTime::now() < *this;
        }

        inline bool isPast() const {
            this->_check_not_zero();
            return *this < SystemTime::now();
        }

        time_t to_time_t() const;

        std::optional<Duration> operator-(const SystemTime& other) const {
            return this->durationSince(other);
        }

        SystemTime operator+(const Duration& dur) const;
        std::strong_ordering operator<=>(const SystemTime& other) const;

    private:
        constexpr SystemTime(i64 _s, i64 _s2) : _storage1(_s), _storage2(_s2) {}

        static SystemTime _epoch();
        void _check_not_zero() const;

        i64 _storage1;
        i64 _storage2;
    };
}
