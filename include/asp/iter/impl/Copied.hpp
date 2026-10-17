#pragma once
#include "../Iter.hpp"

namespace asp::iter {

template <typename T>
using Unreference = std::conditional_t<
    IsReferenceWrapper<T>,
    ExtractRefWrapper<T>,
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

    size_t sizeHint() const requires(ProvidesSizeHint<It>) {
        return m_iter.sizeHint();
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