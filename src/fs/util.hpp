#pragma once
#include <string_view>

namespace asp::fs {

inline auto startsWithOneOf(std::string_view str, std::string_view chars) {
    return !str.empty() && chars.contains(str.front());
}

inline auto endsWithOneOf(std::string_view str, std::string_view chars) {
    return !str.empty() && chars.contains(str.back());
}

}
