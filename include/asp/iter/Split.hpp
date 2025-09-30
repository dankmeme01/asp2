#pragma once
#include <string_view>
#include <span>
#include "Iter.hpp"

namespace asp::iter {

template <typename T, typename D>
T _nextSplit(T& inp, const D& delim);

template <typename T, typename D>
class Split : public Iter<Split<T, D>, T> {
public:
    using Item = T;
    using Delim = D;

    Split(T input, Delim delim) : m_input(std::move(input)), m_delim(std::move(delim)) {}

    std::optional<Item> next() {
        if (m_input.empty()) {
            return std::nullopt;
        }

        return _nextSplit(m_input, m_delim);
    }

private:
    T m_input;
    Delim m_delim;
};

inline auto split(std::string_view sv, char delim) {
    return Split<std::string_view, char>(std::move(sv), std::move(delim));
}

inline auto split(std::string_view sv, std::string_view delim) {
    return Split<std::string_view, std::string_view>(std::move(sv), std::move(delim));
}

inline auto split(const std::vector<unsigned char>& sv, unsigned char delim) {
    return Split<std::span<const unsigned char>, unsigned char>(std::span<const unsigned char>(sv.begin(), sv.end()), std::move(delim));
}

template <>
inline std::string_view _nextSplit(std::string_view& inp, const char& delim) {
    size_t pos = inp.find(delim);
    if (pos == inp.npos) {
        std::string_view out = inp;
        inp = {};
        return out;
    }

    std::string_view out = inp.substr(0, pos);
    inp.remove_prefix(pos + 1);
    return out;
}

template <>
inline std::string_view _nextSplit(std::string_view& inp, const std::string_view& delim) {
    size_t pos = inp.find(delim);
    if (pos == inp.npos) {
        std::string_view out = inp;
        inp = {};
        return out;
    }

    std::string_view out = inp.substr(0, pos);
    inp.remove_prefix(pos + delim.size());
    return out;
}

template <>
inline std::span<const unsigned char> _nextSplit(std::span<const unsigned char>& inp, const unsigned char& delim) {
    size_t pos = 0;
    while (pos < inp.size() && inp[pos] != delim) {
        pos++;
    }

    std::span<const unsigned char> out = inp.subspan(0, pos);
    if (pos < inp.size()) {
        inp = inp.subspan(pos + 1);
    } else {
        inp = {};
    }

    return out;
}

}