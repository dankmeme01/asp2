#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It>
class Skip : public Iter<Skip<It>, typename It::Item> {
public:
    using Item = typename It::Item;

    Skip(It iter, size_t n) : m_iter(std::move(iter)), m_n(n) {}

    std::optional<Item> next() {
        while (m_n > 0) {
            if (!m_iter.next()) {
                return std::nullopt;
            }
            --m_n;
        }

        return m_iter.next();
    }

private:
    It m_iter;
    size_t m_n;
};

template <typename Concrete, typename Item_>
auto Iter<Concrete, Item_>::skip(size_t n) && {
    return Skip<Concrete>(std::move(this->derived()), n);
}

}