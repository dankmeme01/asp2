#pragma once
#include <asp/fs/Error.hpp>
#include <asp/time/SystemTime.hpp>
#include <Windows.h>
#include <fmt/format.h>

namespace asp::fs {

template <typename T>
Result<> wrapCall(T result) {
    if constexpr (std::is_same_v<T, BOOL> || std::is_same_v<T, BOOLEAN>) {
        if (!result) {
            return Err(Error::fromLastOsError());
        }
    } else if constexpr (std::is_same_v<T, HANDLE>) {
        if (result == INVALID_HANDLE_VALUE) {
            return Err(Error::fromLastOsError());
        }
    } else {
        static_assert(std::is_void_v<T>, "wrapCall unsupported type");
    }

    return Ok();
}

static Result<std::string> wideToUtf8(std::wstring_view wide) {
    if (wide.empty()) return Ok(std::string{});

    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
    if (size == 0) {
        return Err(Error::fromLastOsError("Unable to convert to UTF-8"));
    }

    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), utf8.data(), size, nullptr, nullptr);

    return Ok(std::move(utf8));
}

static Result<std::wstring> utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) return Ok(std::wstring{});

    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), (int)utf8.size(), nullptr, 0);
    if (size == 0) {
        return Err(Error::fromLastOsError("Unable to convert to wide string"));
    }

    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), wide.data(), size);

    return Ok(std::move(wide));
}

static SystemTime toSystemTime(FILETIME ft) noexcept {
    // 100ns ticks between 1601-01-01 and 1970-01-01
    static constexpr i64 FILETIME_UNIX_DIFF = 116'444'736'000'000'000;

    ULARGE_INTEGER ticks;
    ticks.LowPart = ft.dwLowDateTime;
    ticks.HighPart = ft.dwHighDateTime;

    i64 unixTicks = (i64)ticks.QuadPart - FILETIME_UNIX_DIFF;
    if (unixTicks < 0) return SystemTime{0, 0};

    i64 secs = unixTicks / 10'000'000;
    i64 rem = unixTicks % 10'000'000;
    if (rem < 0) {
        rem += 10'000'000;
        secs -= 1;
    }

    return SystemTime{secs, (u32)rem * 100};
}


}