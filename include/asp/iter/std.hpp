#pragma once

#include "Iter.hpp"

namespace asp::iter {

template <typename Cont>
auto from(Cont&& cont) {
    using std::begin, std::end;
    return CxxIter(begin(cont), end(cont));
}

template <typename T>
auto from(T* begin, T* end) {
    return CxxIter<T*>(begin, end);
}

template <typename T>
auto from(T* arr, size_t size) {
    return CxxIter<T*>(arr, arr + size);
}

template <typename Cont>
auto fromReverse(Cont&& cont) {
    using std::rbegin, std::rend;
    return CxxIter(rbegin(cont), rend(cont));
}

template <typename Cont>
auto consume(Cont&& cont) {
    using std::begin, std::end;
    return CxxIterConsume(begin(cont), end(cont));
}

template <typename Cont, typename T>
bool contains(Cont&& cont, const T& value) {
    for (auto&& item : cont) {
        if (item == value) {
            return true;
        }
    }

    return false;
}

template <typename T>
bool contains(T* arr, size_t size, const T& value) {
    for (size_t i = 0; i < size; i++) {
        if (arr[i] == value) {
            return true;
        }
    }

    return false;
}

}