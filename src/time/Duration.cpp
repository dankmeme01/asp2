#include <asp/time/Duration.hpp>
#include <asp/Log.hpp>
#include <asp/format.hpp>

#include <stdexcept>

[[noreturn]] void asp::time_detail::_throwrt(const char* msg) {
    asp::log(asp::LogLevel::Error, msg);
    throw std::runtime_error(msg);
}

static void trimTrailingZeros(std::string_view& s) {
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        while (!s.empty() && s.back() == '0')
            s.remove_suffix(1);
        if (!s.empty() && s.back() == '.')
            s.remove_suffix(1);
    }
}

namespace asp::inline time {
    std::string_view suffixForUnit(DurationUnit unit, bool human) {
        switch (unit) {
            case DurationUnit::Nanos:  return human ? " nanoseconds" : "ns";
            case DurationUnit::Micros: return human ? " microseconds" : "µs";
            case DurationUnit::Millis: return human ? " milliseconds" : "ms";
            case DurationUnit::Secs:   return human ? " seconds" : "s";
            case DurationUnit::Mins:   return human ? " minutes" : "min";
            case DurationUnit::Hours:  return human ? " hours" : "h";
            default:                   return "";
        }
    }

    std::pair<std::string_view, DurationUnit> formatDurationNum(
        std::array<char, 32>& buf, const asp::Duration& dur, int precision
    ) {
        using namespace asp::time_detail;
        using namespace asp::nums;

        u64 totalNanos = secs_to_nanos(dur.seconds()) + dur.subsecNanos();

        f64 value;
        DurationUnit unit;

        if (totalNanos >= SECS_IN_HOUR * NANOS_IN_SEC) {
            value = f64(totalNanos) / f64(SECS_IN_HOUR * NANOS_IN_SEC);
            unit = DurationUnit::Hours;
        } else if (totalNanos >= SECS_IN_MIN * NANOS_IN_SEC) {
            value = f64(totalNanos) / f64(SECS_IN_MIN * NANOS_IN_SEC);
            unit = DurationUnit::Mins;
        } else if (totalNanos >= NANOS_IN_SEC) {
            value = f64(totalNanos) / f64(NANOS_IN_SEC);
            unit = DurationUnit::Secs;
        } else if (totalNanos >= NANOS_IN_MILLISEC) {
            value = f64(totalNanos) / f64(NANOS_IN_MILLISEC);
            unit = DurationUnit::Millis;
        } else if (totalNanos >= NANOS_IN_MICROSEC) {
            value = f64(totalNanos) / f64(NANOS_IN_MICROSEC);
            unit = DurationUnit::Micros;
        } else {
            return std::make_pair(asp::local_format(buf, "{}", totalNanos), DurationUnit::Nanos);
        }

        auto num = asp::local_format(buf, "{:.{}f}", value, precision);
        trimTrailingZeros(num);
        return std::make_pair(num, unit);
    }

    template <bool H>
    static std::string formatDur(const Duration& dur, u8 precision) {
        std::array<char, 32> buf;
        auto [num, unit] = formatDurationNum(buf, dur, precision);
        return fmt::format("{}{}", num, suffixForUnit(unit, H));
    }

    std::string Duration::toString(u8 precision) const {
        return formatDur<false>(*this, precision);
    }

    std::string Duration::toHumanString(u8 precision) const {
        auto out = formatDur<true>(*this, precision);
        // this is very silly core
        if (out.starts_with("1 ") && out.ends_with("s")) {
            out.pop_back();
        }
        return out;
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
