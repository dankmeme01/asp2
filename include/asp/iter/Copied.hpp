#pragma once
#include <utility>
#include <optional>
#include "Iter.hpp"

namespace asp::iter {

template <typename T>
struct is_reference_wrapper : std::false_type {};

template <typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};

template <typename T>
struct ExtractRefWrapper {
    using type = T;
};

template <typename T>
struct ExtractRefWrapper<std::reference_wrapper<T>> {
    using type = T;
};

template <typename T>
using Unreference = std::conditional_t<
    is_reference_wrapper<T>::value,
    typename ExtractRefWrapper<T>::type,
    std::remove_reference_t<T>
>;

template <typename T>
using ToCopied = std::remove_cv_t<Unreference<T>>;

template <typename It>
class Copied : public Iter<Copied<It>, ToCopied<typename It::Item>> {
public:
    using Item = ToCopied<typename It::Item>;

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