#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It1, typename It2> requires std::same_as<typename It1::Item, typename It2::Item>
class Chain : public Iter<Chain<It1, It2>, typename It1::Item> {
public:
    using Item = typename It1::Item;

    Chain(It1 iter1, It2 iter2) : m_iter1(std::move(iter1)), m_iter2(std::move(iter2)) {}

    std::optional<Item> next() {
        if (auto item = m_iter1.next()) {
            return item;
        } else if (auto item = m_iter2.next()) {
            return item;
        }

        return std::nullopt;
    }

private:
    It1 m_iter1;
    It2 m_iter2;
};

template <typename Concrete, typename Item_>
template <typename It2>
auto Iter<Concrete, Item_>::chain(It2&& it2) && {
    return Chain<Concrete, std::decay_t<It2>>(
        std::move(this->derived()),
        std::forward<It2>(it2)
    );
}

}