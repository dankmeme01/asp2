#include <asp/time/Duration.hpp>
#include <asp/Log.hpp>

#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace asp::inline time {
    [[noreturn]] void detail::_throwrt(const char* msg) {
        asp::log(asp::LogLevel::Error, msg);
        throw std::runtime_error(msg);
    }

    std::string Duration::toString(u8 precision) const {
        u64 totalNanos = detail::secs_to_nanos(m_seconds) + m_nanos;
        f64 totalSeconds = static_cast<f64>(totalNanos) / static_cast<f64>(detail::NANOS_IN_SEC);

        std::ostringstream oss;

        if (totalSeconds >= detail::SECS_IN_HOUR) {
            f64 hours = totalSeconds / detail::SECS_IN_HOUR;
            oss << std::fixed << std::setprecision(precision) << hours << "h";
        } else if (totalSeconds >= detail::SECS_IN_MIN) {
            f64 mins = totalSeconds / detail::SECS_IN_MIN;
            oss << std::fixed << std::setprecision(precision) << mins << "m";
        } else if (totalSeconds >= 1.0) {
            oss << std::fixed << std::setprecision(precision) << totalSeconds << "s";
        } else if (totalNanos >= detail::NANOS_IN_MILLISEC) {
            f64 millis = (f64)totalNanos / (f64)detail::NANOS_IN_MILLISEC;
            oss << std::fixed << std::setprecision(precision) << millis << "ms";
        } else if (totalNanos >= detail::NANOS_IN_MICROSEC) {
            u64 micros = (f64)totalNanos / (f64)detail::NANOS_IN_MICROSEC;
            oss << micros << "µs";
        } else {
            oss << totalNanos << "ns";
        }

        return oss.str();
    }

    std::string Duration::toHumanString(u8 precision) const {
        // Format duration to a human-readable string, for example 1.484 milliseconds, 3.141 hours, 1.234 seconds
        u64 totalNanos = detail::secs_to_nanos(m_seconds) + m_nanos;
        f64 totalSeconds = static_cast<f64>(totalNanos) / static_cast<f64>(detail::NANOS_IN_SEC);

        std::ostringstream oss;

        if (totalSeconds >= detail::SECS_IN_DAY) {
            f64 days = totalSeconds / detail::SECS_IN_DAY;
            oss << std::fixed << std::setprecision(precision) << days << " days";
        } else if (totalSeconds >= detail::SECS_IN_HOUR) {
            f64 hours = totalSeconds / detail::SECS_IN_HOUR;
            oss << std::fixed << std::setprecision(precision) << hours << " hours";
        } else if (totalSeconds >= detail::SECS_IN_MIN) {
            f64 mins = totalSeconds / detail::SECS_IN_MIN;
            oss << std::fixed << std::setprecision(precision) << mins << " minutes";
        } else if (totalSeconds >= 1.0) {
            oss << std::fixed << std::setprecision(precision) << totalSeconds << " seconds";
        } else if (totalNanos >= detail::NANOS_IN_MILLISEC) {
            f64 millis = (f64)totalNanos / (f64)detail::NANOS_IN_MILLISEC;
            oss << std::fixed << std::setprecision(precision) << millis << " milliseconds";
        } else if (totalNanos >= detail::NANOS_IN_MICROSEC) {
            f64 micros = (f64)totalNanos / (f64)detail::NANOS_IN_MICROSEC;
            oss << std::fixed << std::setprecision(precision) << micros << " microseconds";
        } else {
            oss << totalNanos << " nanoseconds";
        }

        return oss.str();
    }
}

namespace std {
    std::string to_string(const asp::time::Duration& dur) {
        return dur.toString();
    }

    template <> struct hash<asp::time::Duration> {
        std::size_t operator()(const asp::time::Duration& dur) const noexcept {
            return std::hash<asp::u64>{}(dur.nanos());
        }
    };
}
