#include <asp/time/SystemTime.hpp>
#include <asp/detail/config.hpp>

#ifdef ASP_IS_WIN
# include <Windows.h>
#else
# include <time.h>
#endif

namespace asp::time {
    SystemTime SystemTime::UNIX_EPOCH = _epoch();

    SystemTime::SystemTime() : SystemTime(UNIX_EPOCH) {}

    time_t SystemTime::to_time_t() const {
        return this->timeSinceEpoch().seconds();
    }

#ifdef ASP_IS_WIN
    SystemTime SystemTime::now() {
        FILETIME s;
        GetSystemTimePreciseAsFileTime(&s);

        return SystemTime{s.dwHighDateTime, s.dwLowDateTime};
    }

    SystemTime SystemTime::_epoch() {
        SYSTEMTIME epoch;
        epoch.wYear = 1970;
        epoch.wMonth = 1;
        epoch.wDay = 1;
        epoch.wDayOfWeek = 4;
        epoch.wHour = 0;
        epoch.wMilliseconds = 0;
        epoch.wMinute = 0;
        epoch.wSecond = 0;

        FILETIME ue;
        if (!SystemTimeToFileTime(&epoch, &ue)) {
            detail::_throwrt("failed to get unix epoch");
        }

        return SystemTime{ue.dwHighDateTime, ue.dwLowDateTime};
    }

    std::optional<Duration> SystemTime::durationSince(const SystemTime& other) const {
        ULARGE_INTEGER lhs;
        lhs.LowPart = this->_storage2;
        lhs.HighPart = this->_storage1;

        ULARGE_INTEGER rhs;
        rhs.LowPart = other._storage2;
        rhs.HighPart = other._storage1;

        i64 diff = lhs.QuadPart - rhs.QuadPart;

        if (diff < 0) {
            return std::nullopt;
        }

        // convert to 100ns intervals
        return Duration::fromNanos(diff * 100);
    }

    SystemTime SystemTime::operator+(const Duration& dur) const {
        ULARGE_INTEGER uli;
        uli.LowPart = this->_storage2;
        uli.HighPart = this->_storage1;

        // convert to 100ns intervals
        uli.QuadPart += dur.nanos() / 100;

        return SystemTime{uli.HighPart, uli.LowPart};
    }

    std::strong_ordering SystemTime::operator<=>(const SystemTime& other) const {
        ULARGE_INTEGER lhs;
        lhs.LowPart = this->_storage2;
        lhs.HighPart = this->_storage1;

        ULARGE_INTEGER rhs;
        rhs.LowPart = other._storage2;
        rhs.HighPart = other._storage1;

        return lhs.QuadPart <=> rhs.QuadPart;
    }

#else
    SystemTime SystemTime::now() {
        timespec tp;
        if (0 != clock_gettime(CLOCK_REALTIME, &tp)) [[unlikely]] {
            detail::_throwrt("failed to get the current time");
        }

        return SystemTime{tp.tv_sec, tp.tv_nsec};
    }

    SystemTime SystemTime::_epoch() {
        return SystemTime{0, 0};
    }

    std::optional<Duration> SystemTime::durationSince(const SystemTime& other) const {
        i64 secs = this->_storage1 - other._storage1;
        i64 nanos = this->_storage2 - other._storage2;

        if (nanos < 0) {
            nanos += detail::NANOS_IN_SEC;
            secs -= 1;
        }

        if (secs < 0) {
            return std::nullopt;
        }

        return Duration{static_cast<u64>(secs), static_cast<u32>(nanos)};
    }

    SystemTime SystemTime::operator+(const Duration& dur) const {
        i64 secs = this->_storage1 + dur.seconds();
        i64 nanos = this->_storage2 + dur.subsecNanos();

        if (nanos >= detail::NANOS_IN_SEC) {
            nanos -= detail::NANOS_IN_SEC;
            secs += 1;
        }

        return SystemTime{secs, nanos};
    }

    std::strong_ordering SystemTime::operator<=>(const SystemTime& other) const {
        if (this->_storage1 < other._storage1) {
            return std::strong_ordering::less;
        } else if (this->_storage1 > other._storage1) {
            return std::strong_ordering::greater;
        }

        if (this->_storage2 < other._storage2) {
            return std::strong_ordering::less;
        } else if (this->_storage2 > other._storage2) {
            return std::strong_ordering::greater;
        }

        return std::strong_ordering::equal;
    }
#endif
}