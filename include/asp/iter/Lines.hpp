#pragma once
#include <string_view>
#include "Iter.hpp"

namespace asp::iter {

template <typename T>
T _nextLine(T& inp);

template <typename T>
class Lines : public Iter<Lines<T>, T> {
public:
    using Item = T;

    Lines(T input) : m_input(std::move(input)) {}

    std::optional<Item> next() {
        if (m_input.empty()) {
            return std::nullopt;
        }

        return _nextLine(m_input);
    }

private:
    T m_input;
};

/// Returns the iterator over the lines in the string, split at either `\n` or `\r\n` endings.
/// Line terminators are not included in the produced strings. Any standalone `\r` characters do not count as line ends.
/// The string may or may not end with a line terminator, making no difference.
/// An empty string produces no elements.
/// An substring with multiple line terminators in a row will yield empty strings between them.
inline auto lines(std::string_view sv) {
    return Lines<std::string_view>(sv);
}

template <>
inline std::string_view _nextLine(std::string_view& inp) {
    if (inp.empty()) {
        return {};
    }

    size_t startOffset = 0;
    while (true) {
        size_t pos = inp.find_first_of("\r\n", startOffset);
        if (pos == inp.npos) {
            // no line terminator, simply return remainder
            std::string_view out = inp;
            inp = {};
            return out;
        }

        char firstTermChar = inp[pos];
        if (firstTermChar == '\n') {
            // \n is a simple case, line terminates right here
            std::string_view out = inp.substr(0, pos);
            inp.remove_prefix(pos + 1);
            return out;
        }

        // we ran into an \r, if it isn't followed by an \n then proceed forward
        if (pos == inp.size() - 1) {
            // the \r is the final character in the string, return entire string
            std::string_view out = inp;
            inp = {};
            return out;
        }

        char secondTermChar = inp[pos + 1];
        if (secondTermChar == '\n') {
            // we have a \r\n sequence, line terminates here
            std::string_view out = inp.substr(0, pos);
            inp.remove_prefix(pos + 2);
            return out;
        }

        // otherwise, we have an \r followed by a regular character.
        // this means there is no line terminator here, continue searching
        startOffset = pos + 1;
    }
}

}