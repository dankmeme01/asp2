#pragma once

#include "detail.hpp"
#include <optional>
#include "Duration.hpp"

namespace asp::time {
    class SystemTime {
    public:
        constexpr SystemTime(const SystemTime& other) = default;
        constexpr SystemTime& operator=(const SystemTime& other) = default;
        constexpr SystemTime(SystemTime&& other) = default;
        constexpr SystemTime& operator=(SystemTime&& other) = default;

        static SystemTime now();
        static SystemTime UNIX_EPOCH;

        std::optional<Duration> durationSince(const SystemTime& other) const;

        inline Duration timeSinceEpoch() const {
            // we assume that this operation is infallible
            return this->durationSince(UNIX_EPOCH).value();
        }

        // Return the amount of time passed since this measurement was taken until now.
        inline Duration elapsed() const {
            return SystemTime::now().durationSince(*this).value_or(Duration{});
        }

    private:
        constexpr SystemTime(i64 _s, i64 _s2) : _storage1(_s), _storage2(_s2) {}

        static SystemTime _epoch();

        i64 _storage1;
        i64 _storage2;
    };
}