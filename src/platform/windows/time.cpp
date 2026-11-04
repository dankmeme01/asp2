#include <asp/time/SystemTime.hpp>
#include <asp/detail/config.hpp>
#include <ctime>
#include <cstdlib>
#include "helpers.hpp"

namespace asp::inline time {

std::tm localtime(std::time_t time) noexcept {
    std::tm tm{};
    localtime_s(&tm, &time);
    return tm;
}

SystemTime SystemTime::now() noexcept {
    FILETIME s;
    GetSystemTimePreciseAsFileTime(&s);
    return toSystemTime(s);
}

}
