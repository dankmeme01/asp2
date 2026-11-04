#include <asp/time/SystemTime.hpp>
#include <asp/detail/config.hpp>
#include <asp/detail/Result.hpp>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <ctime>
#include <cstdlib>
#include <charconv>

#ifdef _WIN32
static std::tm getTm(const asp::time::SystemTime& st) noexcept {
    std::tm tm{};
    auto timet = st.to_time_t();
    gmtime_s(&tm, &timet);
    return tm;
}
#else
static std::tm getTm(const asp::time::SystemTime& st) noexcept {
    std::tm tm{};
    auto timet = st.to_time_t();
    gmtime_r(&timet, &tm);
    return tm;
}
#endif

namespace asp::inline time {

time_t SystemTime::to_time_t() const noexcept {
    return this->timeSinceEpoch().seconds();
}

Date SystemTime::dateUtc() const noexcept {
    auto tm = getTm(*this);
    return Date{
        static_cast<u32>(tm.tm_year + 1900),
        static_cast<u8>(tm.tm_mon + 1),
        static_cast<u8>(tm.tm_mday),
        static_cast<u8>(tm.tm_wday)
    };
}

Time SystemTime::timeUtc() const noexcept {
    auto tm = getTm(*this);
    return Time{
        static_cast<u8>(tm.tm_hour),
        static_cast<u8>(tm.tm_min),
        static_cast<u8>(tm.tm_sec),
    };
}

DateTime SystemTime::dateTimeUtc() const noexcept {
    return DateTime{
        this->dateUtc(),
        this->timeUtc()
    };
}

std::string SystemTime::format(std::string_view fmt) const {
    time_t curTime = this->to_time_t();
    auto ms = this->timeSinceEpoch().subsecMillis();
    return fmt::format(fmt::runtime(fmt), asp::localtime(curTime), ms);
}

struct IsoParser {
    static constexpr size_t npos = (size_t)-1;

    std::string_view m_str;
    TzPolicy m_tzPolicy;

    char next() {
        auto out = peek();
        if (!m_str.empty()) m_str.remove_prefix(1);
        return out;
    }

    char peek() const {
        if (m_str.empty()) return '\0';
        return m_str.front();
    }

    template <typename T>
    Result<std::pair<T, size_t>> nextIntWithLength(size_t clen = npos) {
        if (clen != npos && m_str.size() < clen) return Err("expected number");
        auto substr = clen == npos ? m_str : m_str.substr(0, clen);

        T value;
        auto [ptr, ec] = std::from_chars(substr.data(), substr.data() + substr.size(), value);
        if (ec != std::errc{}) {
            return Err("failed to parse number");
        }

        size_t parsedLen = ptr - substr.data();
        if (clen != npos && parsedLen != clen) {
            return Err("failed to parse number");
        }

        m_str.remove_prefix(parsedLen);
        return Ok(std::make_pair(value, parsedLen));
    }

    template <typename T>
    Result<T> nextInt(size_t clen = npos) {
        GEODE_UNWRAP_INTO(auto pair, nextIntWithLength<T>(clen));
        return Ok(pair.first);
    }

    Result<char> nextChar(char expected) {
        auto c = next();
        if (c != expected) return Err("expected separator character");
        return Ok(c);
    }

    Result<std::optional<u32>> maybeParseSubsec() {
        if (peek() != '.') return Ok(std::nullopt);
        next();

        GEODE_UNWRAP_INTO(auto pair, nextIntWithLength<u32>());
        auto [fraction, fractionLen] = pair;

        // convert the fraction to nanos
        if (fractionLen > 9) {
            return Err("too many digits in subsecond fraction");
        }

        for (size_t i = fractionLen; i < 9; ++i) {
            fraction *= 10;
        }

        return Ok(fraction);
    }

    Result<i64> parseTzOffset() {
        auto sign = next();
        if (sign != '+' && sign != '-') {
            return Err("expected timezone offset sign");
        }

        GEODE_UNWRAP_INTO(auto hour, nextInt<uint32_t>(2));
        if (peek() == ':') next();
        GEODE_UNWRAP_INTO(auto minute, nextInt<uint32_t>(2));

        if (hour > 23) return Err("invalid timezone hour");
        if (minute > 59) return Err("invalid timezone minute");

        i64 totalOffset = static_cast<i64>(hour) * 3600 + static_cast<i64>(minute) * 60;
        if (sign == '-') {
            totalOffset = -totalOffset;
        }

        return Ok(totalOffset);
    }

    /// Parses as time since unix epoch
    Result<Duration> parseIso8601() {
        GEODE_UNWRAP_INTO(auto year, nextInt<uint32_t>(4));
        GEODE_UNWRAP(nextChar('-'));
        GEODE_UNWRAP_INTO(auto month, nextInt<uint32_t>(2));
        GEODE_UNWRAP(nextChar('-'));
        GEODE_UNWRAP_INTO(auto day, nextInt<uint32_t>(2));

        auto timeSeparator = next();
        if (timeSeparator != 'T' && timeSeparator != ' ') {
            return Err("expected time separator");
        }

        GEODE_UNWRAP_INTO(auto hour, nextInt<uint32_t>(2));
        GEODE_UNWRAP(nextChar(':'));
        GEODE_UNWRAP_INTO(auto minute, nextInt<uint32_t>(2));
        GEODE_UNWRAP(nextChar(':'));
        GEODE_UNWRAP_INTO(auto second, nextInt<uint32_t>(2));

        GEODE_UNWRAP_INTO(auto subsecNanos, maybeParseSubsec());

        i64 tzOffset = 0;
        if (peek() == 'Z') {
            next();
        } else if (m_tzPolicy == TzPolicy::AssumeUtc && m_str.empty()) {
            // do nothing, assume UTC
        } else {
            GEODE_UNWRAP_INTO(tzOffset, parseTzOffset());
        }

        if (!m_str.empty()) {
            return Err("unexpected characters at end of string");
        }

        // validate everything
        if (month < 1 || month > 12) return Err("invalid month");
        if (day < 1 || day > daysInMonth(year, month)) return Err("invalid day");
        if (hour > 23) return Err("invalid hour");
        if (minute > 59) return Err("invalid minute");

        // currently we do not support pre-unix epoch times
        if (year < 1970) return Err("year before unix epoch");

        // leap seconds
        if (second > 60) return Err("invalid second");

        i64 timestamp = 0;
        timestamp += ymdToDays(year, month, day) * 86400;
        timestamp += hour * 3600;
        timestamp += minute * 60;
        timestamp += second;
        timestamp -= tzOffset;

        if (timestamp < 0) return Err("time before unix epoch");

        return Ok(Duration{(u64)timestamp, subsecNanos.value_or(0)});
    }

    static bool isLeapYear(uint32_t year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    static uint32_t daysInMonth(uint32_t year, uint32_t month) {
        static const uint32_t daysInMonth[] = {
            31, 28, 31, 30, 31, 30,
            31, 31, 30, 31, 30, 31
        };

        if (month < 1 || month > 12) return 0;

        if (month == 2 && isLeapYear(year)) {
            return 29;
        }

        return daysInMonth[month - 1];
    }

    static int64_t ymdToDays(u32 y, u32 m, u32 d) {
        // What
        y -= m <= 2;
        u32 era = y / 400;
        u32 yoe = y - era * 400;
        u32 doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
        u32 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return (i64)era * 146097 + doe - 719468;
    }
};

std::optional<SystemTime> SystemTime::parse(std::string_view str, TzPolicy tzPolicy) {
    IsoParser p{str, tzPolicy};
    auto result = p.parseIso8601();
    if (!result) return std::nullopt;

    return SystemTime::UNIX_EPOCH + result.unwrap();
}

std::optional<Duration> SystemTime::durationSince(const SystemTime& other) const noexcept {
    i64 secs = this->m_secs - other.m_secs;
    i64 nanos = (i64)this->m_nanos - (i64)other.m_nanos;

    if (nanos < 0) {
        nanos += time_detail::NANOS_IN_SEC;
        secs -= 1;
    }

    if (secs < 0) {
        return std::nullopt;
    }

    return Duration{static_cast<u64>(secs), static_cast<u32>(nanos)};
}

SystemTime SystemTime::operator+(const Duration& dur) const noexcept {
    i64 secs = this->m_secs + (i64)dur.seconds();
    i64 nanos = this->m_nanos + dur.subsecNanos();

    if (nanos >= time_detail::NANOS_IN_SEC) {
        nanos -= time_detail::NANOS_IN_SEC;
        secs += 1;
    }

    return SystemTime{secs, (u32)nanos};
}

std::strong_ordering SystemTime::operator<=>(const SystemTime& other) const noexcept {
    if (this->m_secs < other.m_secs) {
        return std::strong_ordering::less;
    } else if (this->m_secs > other.m_secs) {
        return std::strong_ordering::greater;
    }

    if (this->m_nanos < other.m_nanos) {
        return std::strong_ordering::less;
    } else if (this->m_nanos > other.m_nanos) {
        return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

}
