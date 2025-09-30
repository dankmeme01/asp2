#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It>
class Cycle : public Iter<Cycle<It>, typename It::Item> {
public:
    using Item = typename It::Item;

    Cycle(It iter) : m_original(std::move(iter)), m_iter(m_original) {}

    std::optional<Item> next() {
        if (auto item = m_iter.next()) {
            return item;
        }

        m_iter = m_original;
        return m_iter.next();
    }

private:
    It m_original;
    It m_iter;
};

template <typename Concrete, typename Item_>
auto Iter<Concrete, Item_>::cycle() && {
    return Cycle<Concrete>(
        std::move(this->derived())
    );
}

}