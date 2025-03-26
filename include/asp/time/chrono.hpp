#pragma once

#include "Duration.hpp"
#include "SystemTime.hpp"
#include <chrono>

namespace asp::time {
    // Converting durations

    template <typename Rep, typename Period>
    std::chrono::duration<Rep, Period> toChrono(const Duration& dur) {
        f64 ratio = static_cast<f64>(Period::num) / Period::den;
        f64 durSeconds = dur.seconds<f64>();

        std::chrono::duration<Rep, Period> chronoDur(static_cast<Rep>(durSeconds / ratio));
        return chronoDur;
    }

    template <typename T>
    T toChrono(const Duration& dur) {
        return toChrono<T::rep, T::period>(dur);
    }

    // Converting SystemTime

    template <typename Clock = std::chrono::high_resolution_clock, typename Duration = typename Clock::duration>
    std::chrono::time_point<Clock, Duration> toChrono(const SystemTime& st) {
        auto dur = st.timeSinceEpoch();
        return std::chrono::time_point<Clock>(toChrono<Duration>(dur));
    }
}