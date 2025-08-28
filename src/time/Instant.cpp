#include <asp/time/Instant.hpp>

#ifdef ASP_IS_WIN
# include <Windows.h>
#else
# include <time.h>
#endif

namespace asp::time {
#ifdef ASP_IS_WIN
    static LARGE_INTEGER getFrequency() {
        static LARGE_INTEGER freq = []() -> LARGE_INTEGER {
            LARGE_INTEGER f;
            if (!QueryPerformanceFrequency(&f)) {
                detail::_throwrt((std::string("failed to get the performance frequency: ") + std::to_string(GetLastError())).c_str());
            }
            return f;
        }();

        return freq;
    }

    Instant Instant::now() {
        LARGE_INTEGER ticks;

        if (!QueryPerformanceCounter(&ticks)) {
            detail::_throwrt((std::string("failed to get the performance counter: ") + std::to_string(GetLastError())).c_str());
        }

        return Instant{ticks.QuadPart};
    }

    Duration Instant::durationSince(const Instant& other) const {
        // get the tick frequency
        LARGE_INTEGER freq = getFrequency();

        // get amount of elapsed ticks
        i64 elapsed = this->_storage - other._storage;
        if (elapsed < 0) {
            elapsed = 0;
        }

        // use 64-bit arithmetic only if the result fits
        if (static_cast<u64>(elapsed) <= (std::numeric_limits<u64>::max() / 1'000'000'000ULL)) {
            u64 nanos = (static_cast<u64>(elapsed) * 1'000'000'000ULL) / static_cast<u64>(freq.QuadPart);
            return Duration::fromNanos(nanos);
        }

        // use doubles otherwise
        double nanosD = (static_cast<double>(elapsed) * 1'000'000'000.0) / static_cast<double>(freq.QuadPart);
        return Duration::fromNanos(static_cast<u64>(nanosD));
    }

    i64 Instant::rawNanos() const {
        return this->durationSince(Instant{0}).nanos();
    }

    static Instant Instant::fromRawNanos(i64 nanos) {
        return Instant{nanos};
    }
#else
    Instant Instant::now() {
        timespec tp;
        if (0 != clock_gettime(CLOCK_MONOTONIC_RAW, &tp)) [[unlikely]] {
            detail::_throwrt("failed to get the current time");
        }

        return Instant{tp.tv_sec, tp.tv_nsec};
    }

    Duration Instant::durationSince(const Instant& other) const {
        i64 secs = this->_storage1 - other._storage1;
        i64 nanos = this->_storage2 - other._storage2;

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
        return this->durationSince(Instant{0, 0}).nanos();
    }
#endif
}