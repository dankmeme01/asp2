#include <asp/time/sleep.hpp>
#include <asp/time/chrono.hpp>
#include <asp/time/Duration.hpp>
#include <asp/time/SystemTime.hpp>
#include <thread>

namespace asp::time {
    void sleep(const Duration& dur) {
        auto val = asp::time::toChrono<std::chrono::microseconds>(dur);
        std::this_thread::sleep_for(val);
    }

    void sleepUntil(const SystemTime& st) {
        auto tp = asp::time::toChrono(st);
        std::this_thread::sleep_until(tp);
    }

    void yield() {
        std::this_thread::yield();
    }
}
