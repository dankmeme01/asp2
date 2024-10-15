#include <asp/time/Instant.hpp>

#ifdef ASP_IS_WIN
# include <Windows.h>
#else
# include <time.h>
#endif

namespace asp::time {
#ifdef ASP_IS_WIN
    Instant Instant::now() {
        LARGE_INTEGER ticks;

        if (!QueryPerformanceCounter(&ticks)) {
            detail::_throwrt((std::string("failed to get the performance counter: ") + std::to_string(GetLastError())).c_str());
        }

        return Instant{ticks.QuadPart};
    }

    Duration Instant::durationSince(const Instant& other) const {
        // get the tick frequency
        static LARGE_INTEGER freq = []() -> LARGE_INTEGER {
            LARGE_INTEGER f;
            if (!QueryPerformanceFrequency(&f)) {
                detail::_throwrt((std::string("failed to get the performance frequency: ") + std::to_string(GetLastError())).c_str());
            }

            return f;
        }();

        // get amount of elapsed ticks
        i64 elapsed = this->_storage - other._storage;
        if (elapsed < 0) {
            elapsed = 0;
        }

        // convert to an amount of microseconds, then to a duration
        u64 micros = (static_cast<u64>(elapsed) * detail::NANOS_IN_MICROSEC) / static_cast<u64>(freq.QuadPart);

        return Duration::fromMicros(micros);
    }
#else
    Instant Instant::now() {
        timespec tp;
        if (0 != clock_gettime(CLOCK_MONOTONIC, &tp)) [[unlikely]] {
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
#endif
}