#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It>
class Copied : public Iter<Copied<It>, std::remove_reference_t<typename It::Item>> {
public:
    using Item = std::remove_reference_t<typename It::Item>;

    Copied(It iter) : m_iter(std::move(iter)) {}

    std::optional<Item> next() {
        if (auto item = m_iter.next()) {
            return *item;
        }

        return std::nullopt;
    }

private:
    It m_iter;
};

template <typename Concrete, typename Item_>
auto Iter<Concrete, Item_>::copied() && {
    return Copied<Concrete>(
        std::move(this->derived())
    );
}

}