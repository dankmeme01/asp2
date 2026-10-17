#pragma once
#include "../Iter.hpp"

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

    size_t sizeHint() const requires(ProvidesSizeHint<It>) {
        size_t hint = m_iter.sizeHint();
        if (hint < m_n) {
            return 0;
        } else {
            return hint - m_n;
        }
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