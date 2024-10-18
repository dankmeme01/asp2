#pragma once

#include <asp/detail/config.hpp>
#include <variant>
#include <string>
#include <string_view>

namespace asp::data {
    // concept that determines if OwnedT can be directly converted into BorrowedT
    template <typename OwnedT, typename BorrowedT>
    concept CowConvertible = requires(OwnedT owned) {
        { static_cast<BorrowedT>(owned) } -> std::convertible_to<BorrowedT>;
    };

    // A Cow is a data structure that can hold either an owned value or a borrowed value.
    template <typename OwnedT, typename BorrowedT>
    class Cow {
    public:
        using Owned = OwnedT;
        using Borrowed = BorrowedT;

        constexpr Cow(const Cow& other) = default;
        constexpr Cow& operator=(const Cow& other) = default;
        constexpr Cow(Cow&& other) = default;
        constexpr Cow& operator=(Cow&& other) = default;

        constexpr Cow(Owned&& value) : _storage(std::move(value)) {}
        constexpr Cow(const Owned& value) : _storage(value) {}
        constexpr Cow(Borrowed&& value) : _storage(value) {}
        constexpr Cow(const Borrowed& value) : _storage(value) {}

        constexpr static Cow fromOwned(Owned&& value) {
            return Cow(std::move(value));
        }

        constexpr static Cow fromOwned(const Owned& value) {
            return Cow(value);
        }

        constexpr static Cow fromBorrowed(Borrowed&& value) {
            return Cow(std::move(value));
        }

        constexpr static Cow fromBorrowed(const Borrowed& value) {
            return Cow(value);
        }

        constexpr Cow& operator=(Owned&& value) {
            _storage = std::move(value);
            return *this;
        }

        constexpr Cow& operator=(const Owned& value) {
            _storage = value;
            return *this;
        }

        constexpr Cow& operator=(Borrowed&& value) {
            _storage = std::move(value);
            return *this;
        }

        constexpr Cow& operator=(const Borrowed& value) {
            _storage = value;
            return *this;
        }

        bool isOwned() const {
            return std::holds_alternative<Owned>(_storage);
        }

        bool isBorrowed() const {
            return std::holds_alternative<Borrowed>(_storage);
        }

        Owned& getOwned() {
            return std::get<Owned>(_storage);
        }

        const Owned& getOwned() const {
            return std::get<Owned>(_storage);
        }

        // if Owned cannot be directly converted to Borrowed, then this function only works if we hold Borrowed Data
        Borrowed& getBorrowed() requires (!CowConvertible<Owned, Borrowed>) {
            return std::get<Borrowed>(_storage);
        }

        // const version
        const Borrowed& getBorrowed() const requires (!CowConvertible<Owned, Borrowed>) {
            return std::get<Borrowed>(_storage);
        }

        // otherwise, this function will convert Owned to Borrowed and return it
        Borrowed getBorrowed() const requires CowConvertible<Owned, Borrowed> {
            if (isOwned()) {
                return static_cast<Borrowed>(std::get<Owned>(_storage));
            }

            return std::get<Borrowed>(_storage);
        }

    private:
        std::variant<Owned, Borrowed> _storage;
    };

    using CowString = Cow<std::string, std::string_view>;
}

// int test() {
//     // both are valid here, `y` allocates, `x` does not
//     auto x = asp::data::CowString::fromBorrowed("hi there");
//     auto y = asp::data::CowString::fromOwned("hi there");
//
//     // both return a string_view here
//     auto xsv = x.getBorrowed();
//     auto ysv = y.getBorrowed();
// }