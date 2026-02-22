#pragma once

#include "detail/config.hpp"
#include "detail/Function.hpp"
#include <string_view>
#include <fmt/core.h>

namespace asp {
    enum class LogLevel {
        Trace, Debug, Info, Warn, Error
    };

    void setLogFunction(asp::MoveOnlyFunction<void(LogLevel, const std::string_view)>&& f);
    asp::MoveOnlyFunction<void(LogLevel, const std::string_view)>& getLogFunction();

    void doLog(LogLevel level, const std::string_view message);

    inline void log(LogLevel level, const std::string_view message) {
    #ifndef ASP_DEBUG
        // disable trace logs in release
        if (level == LogLevel::Trace) return;
    #endif
        doLog(level, message);
    }

    inline void trace(const std::string_view message) {
        log(LogLevel::Trace, message);
    }

    template <class... Args>
    inline void log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) {
    #ifndef ASP_DEBUG
        // disable trace logs in release
        if (level == LogLevel::Trace) return;
    #endif
        doLog(level, fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <class... Args>
    inline void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Trace, fmt, std::forward<Args>(args)...);
    }
}