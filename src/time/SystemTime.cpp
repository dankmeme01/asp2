#include <asp/time/SystemTime.hpp>
#include <asp/detail/config.hpp>

#ifdef ASP_IS_WIN
# include <Windows.h>
#else
# include <time.h>
#endif

namespace asp::time {
    SystemTime SystemTime::UNIX_EPOCH = _epoch();

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
#endif
}