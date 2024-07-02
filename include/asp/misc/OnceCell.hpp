// this is kinda useless but i left it here anyway

#pragma once

#include <asp/detail/config.hpp>
#include <type_traits>
#include <new>
#include <optional>

namespace asp {
    template <typename Ret, typename Func>
    concept OnceCellFunction = requires(Func func) {
        { std::is_invocable_r_v<Ret, Func> };
    };

    template <std::move_constructible Ret>
    class OnceCell {
    public:
        OnceCell() {}

        // Get the underlying object. Throws an exception if the cell is not initialized.
        Ret& get() const {
            ASP_ALWAYS_ASSERT(initialized, "OnceCell::get called on an uninitialized cell");

            return getUnchecked();
        }

        // Get the underlying object, or a nullopt optional if the cell is not initialized.
        std::optional<std::reference_wrapper<Ret>> tryGet() const {
            if (initialized) {
                return std::optional(getUnchecked());
            } else {
                return std::nullopt;
            }
        }

        // Set the underlying object, does nothing if the cell is already initialized.
        void set(Ret&& value) {
            if (initialized) return;

            new (memory) Ret(std::move(value));
            initialized = true;
        }

        // Set the underlying object, does nothing if the cell is already initialized.
        std::enable_if_t<std::is_copy_constructible_v<Ret>, void> set(const Ret& value) {
            if (initialized) return;

            new (memory) Ret(value);
            initialized = true;
        }

        // Get the underlying object. The behavior is undefined if the cell is not initialized.
        Ret& getUnchecked() const {
            ASP_ASSERT(initialized, "OnceCell::getUnchecked called on an uninitialized cell");

            return *std::launder<Ret*>(memory);
        }

        // Get the underlying object if initialized, otherwise initialize it using the given function
        template <typename Initer> requires OnceCellFunction<Ret, Initer>
        Ret& getOrInit(const Initer& initer) {
            if (!initialized) {
                this->set(initer());
            }

            return getUnchecked();
        }

        // Get the underlying object if initialized, otherwise initialize it using the given function
        template <typename Initer> requires OnceCellFunction<Ret, Initer>
        Ret& getOrInit(Initer&& initer) {
            if (!initialized) {
                this->set(initer());
            }

            return getUnchecked();
        }

    private:
        alignas(Ret) unsigned char memory[sizeof(Ret)];
        bool initialized = false;
    };
}