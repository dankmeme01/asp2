#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It1, typename It2>
class Zip : public Iter<Zip<It1, It2>, std::pair<typename It1::Item, typename It2::Item>> {
public:
    using Item = std::pair<typename It1::Item, typename It2::Item>;

    Zip(It1 iter1, It2 iter2) : m_iter1(std::move(iter1)), m_iter2(std::move(iter2)) {}

    std::optional<Item> next() {
        auto item1 = m_iter1.next();
        auto item2 = m_iter2.next();

        if (item1 && item2) {
            return std::make_pair(std::move(*item1), std::move(*item2));
        }

        return std::nullopt;
    }

private:
    It1 m_iter1;
    It2 m_iter2;
};

template <typename Concrete, typename Item_>
template <typename It2>
auto Iter<Concrete, Item_>::zip(It2&& it2) && {
    return Zip<Concrete, std::decay_t<It2>>(
        std::move(this->derived()),
        std::forward<It2>(it2)
    );
}

}