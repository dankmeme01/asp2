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

static asp::SystemTime toSystemTime(FILETIME ft) {
    constexpr uint64_t EPOCH_DIFF = 116444736000000000ULL;

    auto ft64 = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    if (ft64 < EPOCH_DIFF) return asp::SystemTime::UNIX_EPOCH;

    auto ms = (ft64 - EPOCH_DIFF) / 10'000ULL;
    return asp::SystemTime::fromUnixMillis(ms);
}

}