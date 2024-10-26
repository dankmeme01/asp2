#pragma once

#include <asp/detail/config.hpp>
#include <variant>
#include <string>
#include <string_view>

namespace asp::data {
    template <typename To, typename From>
    To toBorrowed(const From& from) {
        return static_cast<To>(from);
    }

    template <typename To, typename From>
    To toOwned(const From& from) {
        return static_cast<To>(from);
    }

    // concept that determines if OwnedT can be directly converted into BorrowedT
    template <typename OwnedT, typename BorrowedT>
    concept CowConvertibleCast = requires(OwnedT owned) {
        { static_cast<BorrowedT>(owned) } -> std::convertible_to<BorrowedT>;
    };

    template <typename OwnedT, typename BorrowedT>
    concept CowConvertibleBorrowFn = requires(const OwnedT& owned) {
        { asp::data::toBorrowed(owned) } -> std::convertible_to<BorrowedT>;
    };

    template <typename OwnedT, typename BorrowedT>
    concept CowConvertibleToOwned = requires(const BorrowedT& borrowed) {
        { asp::data::toOwned(borrowed) } -> std::convertible_to<OwnedT>;
    };

    template <typename OwnedT, typename BorrowedT>
    concept CowConvertibleToBorrowed = CowConvertibleCast<OwnedT, BorrowedT> || CowConvertibleBorrowFn<OwnedT, BorrowedT>;

    // A Cow is a data structure that can hold either an owned value or a borrowed value.
    template <typename OwnedT, typename BorrowedT>
    class Cow {
        struct none_t {};

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

        // Getter functions.
        // asXXXX functions will throw if the value is not of the correct type,
        // toXXXX functions will convert the value to borrowed/owned if necessary.

        Owned asOwned() && {
            return std::get<Owned>(std::move(_storage));
        }

        Owned& asOwned() & {
            return std::get<Owned>(_storage);
        }

        const Owned& asOwned() const& {
            return std::get<Owned>(_storage);
        }

        Borrowed asBorrowed() && {
            return std::get<Borrowed>(std::move(_storage));
        }

        Borrowed& asBorrowed() & {
            return std::get<Borrowed>(_storage);
        }

        const Borrowed& asBorrowed() const& {
            return std::get<Borrowed>(_storage);
        }

        // Returns the owned value, converting it from borrowed if necessary.
        // If the value is owned, it will be moved out of the Cow.
        Owned toOwned() && requires CowConvertibleToOwned<Owned, Borrowed> {
            if (isOwned()) {
                return std::get<Owned>(std::move(_storage));
            } else {
                return asp::data::toOwned<Owned>(std::get<Borrowed>(_storage));
            }
        }

        // Returns the owned value, converting it from borrowed if necessary.
        // If the value is owned, it will be copied out of the Cow.
        Owned toOwned() const& requires CowConvertibleToOwned<Owned, Borrowed> {
            if (isOwned()) {
                return std::get<Owned>(_storage);
            } else {
                return asp::data::toOwned<Owned>(std::get<Borrowed>(_storage));
            }
        }

        Borrowed toBorrowed() && requires CowConvertibleToBorrowed<Owned, Borrowed> = delete;

        // Returns the borrowed value, converting it from owned if necessary.
        // If the value is borrowed, it will be copied out of the Cow.
        Borrowed toBorrowed() const& requires CowConvertibleToBorrowed<Owned, Borrowed> {
            if (isBorrowed()) {
                return std::get<Borrowed>(_storage);
            } else {
                return asp::data::toBorrowed<Borrowed>(std::get<Owned>(_storage));
            }
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