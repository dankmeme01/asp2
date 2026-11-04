#include <asp/time/SystemTime.hpp>
#include <asp/detail/config.hpp>
#include <ctime>

namespace asp::inline time {

std::tm localtime(std::time_t time) noexcept {
    std::tm tm{};
    localtime_r(&time, &tm);
    return tm;
}

SystemTime SystemTime::now() noexcept {
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return SystemTime{tp.tv_sec, (u32)tp.tv_nsec};
}

}
