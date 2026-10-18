#pragma once
#include <asp/iter/Iter.hpp>
#include <asp/collections/SmallVec.hpp>

namespace asp::iter {

template <typename T, size_t N>
struct Collector<asp::SmallVec<T, N>> {
    Collector(std::optional<size_t> sizeHint) {
        if (sizeHint) {
            m_container.reserve(*sizeHint);
        }
    }

    template <typename Y>
    void push(Y&& item) {
        m_container.emplace_back(std::forward<Y>(item));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    asp::SmallVec<T, N> m_container;
};

}
