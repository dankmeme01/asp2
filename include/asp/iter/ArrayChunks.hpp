#pragma once
#include <utility>
#include <optional>
#include <functional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It, size_t N>
class ArrayChunks : public Iter<ArrayChunks<It, N>, std::array<typename It::Item, N>> {
public:
    using Item = std::array<typename It::Item, N>;

    ArrayChunks(It iter) : m_iter(std::move(iter)) {}

    std::optional<Item> next() {
        Item arr;
        for (size_t i = 0; i < N; ++i) {
            if (auto item = m_iter.next()) {
                arr[i] = std::move(*item);
            } else {
                return std::nullopt;
            }
        }

        return arr;
    }

private:
    It m_iter;
};

template <typename Concrete, typename Item_>
template <size_t N>
auto Iter<Concrete, Item_>::arrayChunks() && {
    return ArrayChunks<Concrete, N>(
        std::move(this->derived())
    );
}

}