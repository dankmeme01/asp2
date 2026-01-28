#pragma once
#include <asp/data/nums.hpp>

namespace asp::inline time {

/// Represents a calendar date.
/// Month: 1 = January, 12 = December
/// Day: 1..31
/// Weekday: 0 = Sunday, 6 = Saturday
struct Date {
    u32 year;
    u8 month;
    u8 day;
    u8 weekday;
};

struct Time {
    u8 hours;
    u8 minutes;
    u8 seconds;
};

struct DateTime {
    Date date;
    Time time;
};

}