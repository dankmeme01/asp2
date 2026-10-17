#pragma once
#include <asp/detail/Result.hpp>
#include <asp/time/SystemTime.hpp>
#include <Windows.h>
#include <fmt/format.h>

namespace asp {

static std::string formatError(DWORD error = GetLastError()) {
    LPSTR buffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        (LPSTR)&buffer,
        0,
        nullptr
    );

    if (size == 0 || !buffer) {
        return fmt::format("Win error {}", error);
    }

    std::string message(buffer, size);
    LocalFree(buffer);

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    return message;
}

template <typename T>
Result<> wrapCall(T result) {
    if constexpr (std::is_same_v<T, BOOL> || std::is_same_v<T, BOOLEAN>) {
        if (!result) {
            return Err(formatError());
        }
    } else if constexpr (std::is_same_v<T, HANDLE>) {
        if (result == INVALID_HANDLE_VALUE) {
            return Err(formatError());
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
        return Err(fmt::format("Unable to convert string: {}", formatError()));
    }

    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), utf8.data(), size, nullptr, nullptr);

    return Ok(std::move(utf8));
}

static Result<std::wstring> utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) return Ok(std::wstring{});

    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), (int)utf8.size(), nullptr, 0);
    if (size == 0) {
        return Err(fmt::format("Unable to convert string: {}", formatError()));
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