#pragma once
#include "Iter.hpp"

namespace asp::iter {

template <typename T>
class RangeIter : public Iter<RangeIter<T>, T> {
public:
    using Item = T;

    RangeIter(T start, T end) : m_current(start), m_end(end) {}

    std::optional<T> next() {
        if (m_current == m_end) {
            return std::nullopt;
        }

        return m_current++;
    }

private:
    T m_current;
    T m_end;
};

template <typename I>
auto range(I start, I end) {
    return RangeIter<I>(start, end);
}

}
