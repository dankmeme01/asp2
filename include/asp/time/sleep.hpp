#pragma once
#include "chrono.hpp"
#include <thread>

namespace asp::inline time {
    inline void sleep(const Duration& dur) {
        auto val = asp::time::toChrono<std::chrono::microseconds>(dur);
        std::this_thread::sleep_for(val);
    }

    inline void sleepUntil(const SystemTime& st) {
        auto tp = asp::time::toChrono(st);
        std::this_thread::sleep_until(tp);
    }

    inline void yield() {
        std::this_thread::yield();
    }
}
