#pragma once

#include <fmt/format.h>
#include <array>
#include <string_view>
#include <stddef.h>

namespace asp {

template <size_t N, typename... Args>
std::string_view local_format(std::array<char, N>& arr, fmt::format_string<Args...> fmt, Args&&... args) {
    auto size = fmt::format_to_n(arr.data(), N, fmt, std::forward<Args>(args)...).size;

    if (size < N) {
        arr[size] = '\0';
    } else {
        arr[N - 1] = '\0';
    }

    return std::string_view{arr.data(), size};
}

template <size_t N, typename... Args>
std::string_view local_format(char(&arr)[N], fmt::format_string<Args...> fmt, Args&&... args) {
    auto size = fmt::format_to_n(&arr[0], N, fmt, std::forward<Args>(args)...).size;

    if (size < N) {
        arr[size] = '\0';
    } else {
        arr[N - 1] = '\0';
    }

    return std::string_view{&arr[0], size};
}


}