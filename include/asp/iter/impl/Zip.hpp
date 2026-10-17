#pragma once
#include "../Iter.hpp"

namespace asp::iter {

template <typename It1, typename It2>
using ZipItem = std::pair<UnwrapRefWrapper<typename It1::Item>, UnwrapRefWrapper<typename It2::Item>>;

template <typename It1, typename It2>
class Zip : public Iter<Zip<It1, It2>, ZipItem<It1, It2>> {
public:
    using Item = ZipItem<It1, It2>;

    Zip(It1 iter1, It2 iter2) : m_iter1(std::move(iter1)), m_iter2(std::move(iter2)) {}

    std::optional<Item> next() {
        auto item1 = m_iter1.next();
        auto item2 = m_iter2.next();

        if (item1 && item2) {
            return Item{unwrapRefWrapper(std::move(*item1)), unwrapRefWrapper(std::move(*item2))};
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