#pragma once

#include "Iter.hpp"

namespace asp::iter {

template <typename Cont>
auto from(Cont&& cont) {
    using std::begin, std::end;
    return CxxIter(begin(cont), end(cont));
}

template <typename Cont>
auto fromReverse(Cont&& cont) {
    using std::rbegin, std::rend;
    return CxxIter(rbegin(cont), rend(cont));
}

}