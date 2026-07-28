#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"
#include "Util.hpp"

namespace asp::iter {

template <typename T>
struct WrapContainerOrIterator {
    static auto wrap(const T& value) {
        if constexpr (is_reference_wrapper<T>::value) {
            using Inner = ExtractRefWrapper<T>::type;
            return WrapContainerOrIterator<Inner>::wrap(value.get());
        } else if constexpr (std::is_base_of_v<IterBase, T>) {
            // this is an asp iterator, just return it
            return value;
        } else {
            // this is a container
            using std::begin, std::end;
            return CxxIter(begin(value), end(value));
        }
    }

    using type = std::decay_t<decltype(wrap(std::declval<T>()))>;
};

template <
    typename It,
    typename InnerValue = typename It::Item,
    typename InnerIterator = WrapContainerOrIterator<InnerValue>::type,
    typename Item_ = typename InnerIterator::Item
>
class Flatten : public Iter<Flatten<It>, Item_> {
public:
    using Item = Item_;

    Flatten(It iter) : m_iter(std::move(iter)) {}

    std::optional<Item> next() {
        while (true) {
            // if we have an iterable, keep processing it
            if (m_curIter) {
                auto item = m_curIter->next();
                if (item) return item;

                m_curIter = std::nullopt;
            }

            // get a new iterable
            m_curValue = m_iter.next();
            if (m_curValue) {
                m_curIter = WrapContainerOrIterator<InnerValue>::wrap(*m_curValue);
            } else {
                // no more iterables, iterator is exhausted
                return std::nullopt;
            }
        }
    }

private:
    It m_iter;
    std::optional<InnerValue> m_curValue;
    std::optional<InnerIterator> m_curIter;
};

template <typename Concrete, typename Item_>
auto Iter<Concrete, Item_>::flatten() && {
    return Flatten<Concrete>(
        std::move(this->derived())
    );
}

}