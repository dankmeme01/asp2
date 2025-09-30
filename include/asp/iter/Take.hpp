#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It>
class Take : public Iter<Take<It>, typename It::Item> {
public:
    using Item = typename It::Item;

    Take(It iter, size_t n) : m_iter(std::move(iter)), m_n(n) {}

    std::optional<Item> next() {
        if (m_n == 0) {
            return std::nullopt;
        }

        --m_n;
        return m_iter.next();
    }

private:
    It m_iter;
    size_t m_n;
};

template <typename Concrete, typename Item_>
auto Iter<Concrete, Item_>::take(size_t n) && {
    return Take<Concrete>(std::move(this->derived()), n);
}

}