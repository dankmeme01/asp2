// This is copied from the geode library for now, if i feel like dealing with the dependency hell in the future,
// i will integrate it properly with cpm
// https://github.com/geode-sdk/result

#pragma once
#include <concepts>
#include <utility>
#include <stdexcept>
#include <variant>
#include <optional>

#if !defined(GEODE_CONCAT)
    #define GEODE_CONCAT2(x, y) x##y
    #define GEODE_CONCAT(x, y) GEODE_CONCAT2(x, y)
#endif

#if !defined(GEODE_UNWRAP)
    // Use gcc's scope expression feature, which makes this macro
    // really nice to use. Unfortunately not available on MSVC
    #if defined(__GNUC__) || defined(__clang__)
        #define GEODE_UNWRAP(...) ({                                        \
            auto GEODE_CONCAT(res, __LINE__) = __VA_ARGS__;                 \
            if (GEODE_CONCAT(res, __LINE__).isErr())                        \
                return geode::Err(GEODE_CONCAT(res, __LINE__).unwrapErr()); \
            GEODE_CONCAT(res, __LINE__).unwrap();                           \
        })
    #else
        #define GEODE_UNWRAP(...)                                   \
        if (auto res = __VA_ARGS__; res.isErr()) return geode::Err(res.unwrapErr())
    #endif
#endif

#if !defined (GEODE_UNWRAP_INTO)
    #define GEODE_UNWRAP_INTO(variable, ...)                                             \
    auto GEODE_CONCAT(res, __LINE__) = __VA_ARGS__;                                      \
    if (GEODE_CONCAT(res, __LINE__).isErr()) return geode::Err(GEODE_CONCAT(res, __LINE__).unwrapErr()); \
    variable = GEODE_CONCAT(res, __LINE__).unwrap()
#endif

namespace _r {
    template <class OkType, class ErrType>
    class Result;

    namespace impl {
        template <class OkType>
        class OkContainer;

        template <class ErrType>
        class ErrContainer;

        template <class OkType, class ErrType>
        class ResultData;

        template <class OkType, class ErrType>
        class ResultDataWrapper;
    }

    template <class OkType>
    inline impl::OkContainer<OkType> Ok(OkType&& ok);

    template <class ErrType>
    inline impl::ErrContainer<ErrType> Err(ErrType&& err);

    template <class OkType>
    inline impl::OkContainer<OkType&> Ok(OkType& ok);

    template <class ErrType>
    inline impl::ErrContainer<ErrType&> Err(ErrType& err);

    inline impl::OkContainer<void> Ok();

    inline impl::ErrContainer<void> Err();

    namespace impl {
        template <class OkType>
        class OkContainer final {
        protected:
            OkType m_ok;

            explicit OkContainer(OkType&& ok)
                noexcept(std::is_nothrow_constructible_v<OkType, OkType>)
                : m_ok(std::forward<OkType>(ok)) {}

        public:
            OkType unwrap() noexcept(std::is_nothrow_move_constructible_v<OkType>) {
                return std::move(m_ok);
            }

            friend impl::OkContainer<OkType> _r::Ok<OkType>(OkType&& ok);
        };

        template <class OkType>
        class OkContainer<OkType&> final {
        protected:
            OkType& m_ok;

            explicit OkContainer(OkType& ok) noexcept : m_ok(ok) {}

        public:
            std::reference_wrapper<OkType> unwrap() noexcept {
                return m_ok;
            }

            friend impl::OkContainer<OkType&> _r::Ok<OkType>(OkType& ok);
        };

        template <>
        class OkContainer<void> final {
        protected:
            explicit OkContainer() {}

            friend impl::OkContainer<void> _r::Ok();
        };

        template <class ErrType>
        class ErrContainer final {
        protected:
            ErrType m_err;

            explicit ErrContainer(ErrType&& err)
                noexcept(std::is_nothrow_constructible_v<ErrType, ErrType>)
                : m_err(std::forward<ErrType>(err)) {}

        public:
            ErrType unwrap() noexcept(std::is_nothrow_move_constructible_v<ErrType>) {
                return std::move(m_err);
            }

            friend impl::ErrContainer<ErrType> _r::Err<ErrType>(ErrType&& err);
        };

        template <class ErrType>
        class ErrContainer<ErrType&> final {
        protected:
            ErrType& m_err;

            explicit ErrContainer(ErrType& err) noexcept : m_err(err) {}

        public:
            std::reference_wrapper<ErrType> unwrap() noexcept {
                return m_err;
            }

            friend impl::ErrContainer<ErrType&> _r::Err<ErrType>(ErrType& err);
        };

        template <>
        class ErrContainer<void> final {
        protected:
            explicit ErrContainer() {}

            friend impl::ErrContainer<void> _r::Err();
        };

        template <class Type>
        struct IsOptionalImpl : std::false_type {
            using type = void;
        };

        template <class Type>
        struct IsOptionalImpl<std::optional<Type>> : std::true_type {
            using type = Type;
        };

        template <class Type>
        concept IsOptional = IsOptionalImpl<Type>::value;

        template <class Type>
        using OptionalType = typename IsOptionalImpl<Type>::type;

        template <class Type>
        struct IsResultImpl : std::false_type {
            using OkType = void;
            using ErrType = void;
        };

        template <class OkType2, class ErrType2>
        struct IsResultImpl<Result<OkType2, ErrType2>> : std::true_type {
            using OkType = OkType2;
            using ErrType = ErrType2;
        };

        template <class Type>
        concept IsResult = IsResultImpl<Type>::value;

        template <class Type>
        using ResultOkType = typename IsResultImpl<Type>::OkType;

        template <class Type>
        using ResultErrType = typename IsResultImpl<Type>::ErrType;
    }

    /// @brief Constructs a new Ok value
    /// @param ok the value to wrap in an Ok
    /// @return a new Ok value
    template <class OkType>
    impl::OkContainer<OkType> Ok(OkType&& ok) {
        return impl::OkContainer<OkType>(std::forward<OkType>(ok));
    }

    /// @brief Constructs a new Err value
    /// @param err the value to wrap in an Err
    /// @return a new Err value
    template <class ErrType>
    impl::ErrContainer<ErrType> Err(ErrType&& err) {
        return impl::ErrContainer<ErrType>(std::forward<ErrType>(err));
    }

    /// @brief Constructs a new Ok value
    /// @param ok the reference to wrap in an Ok
    /// @return a new Ok value
    template <class OkType>
    impl::OkContainer<OkType&> Ok(OkType& ok) {
        return impl::OkContainer<OkType&>(ok);
    }

    /// @brief Constructs a new Err value
    /// @param err the reference to wrap in an Err
    /// @return a new Err value
    template <class ErrType>
    impl::ErrContainer<ErrType&> Err(ErrType& err) {
        return impl::ErrContainer<ErrType&>(err);
    }

    /// @brief Constructs a new void Ok value
    /// @return a new void Ok value
    impl::OkContainer<void> Ok() {
        return impl::OkContainer<void>();
    }

    /// @brief Constructs a new void Err value
    /// @return a new void Err value
    impl::ErrContainer<void> Err() {
        return impl::ErrContainer<void>();
    }

    namespace impl {

        template <class OkType, class ErrType>
        class ResultData {
        protected:
            using ProtectedOkType = std::conditional_t<std::is_reference_v<OkType>, std::reference_wrapper<std::remove_reference_t<OkType>>, OkType>;
            using ProtectedErrType = std::conditional_t<std::is_reference_v<ErrType>, std::reference_wrapper<std::remove_reference_t<ErrType>>, ErrType>;

            std::variant<ProtectedOkType, ProtectedErrType> m_data;

            template <std::size_t Index, class ClassType>
            requires (Index == 0 ? std::constructible_from<ProtectedOkType, ClassType> : std::constructible_from<ProtectedErrType, ClassType>)
            ResultData(std::in_place_index_t<Index> index, ClassType&& ok)
                noexcept(Index == 0 ? std::is_nothrow_constructible_v<ProtectedOkType, ClassType> : std::is_nothrow_constructible_v<ProtectedErrType, ClassType>)
                : m_data(index, std::move(ok)) {}

            ResultData(ResultData&& other)
                noexcept(std::is_nothrow_move_constructible_v<ProtectedOkType> && std::is_nothrow_move_constructible_v<ProtectedErrType>)
                : m_data(std::move(other.m_data)) {}

            OkContainer<OkType> asOk() noexcept
                requires(!std::is_reference_v<OkType>) {
                return Ok(std::move(std::get<0>(m_data)));
            }

            OkContainer<OkType&> asOk() noexcept
                requires(std::is_reference_v<OkType>) {
                return Ok(std::get<0>(m_data).get());
            }

            ErrContainer<ErrType> asErr() noexcept
                requires(!std::is_reference_v<ErrType>) {
                return Err(std::move(std::get<1>(m_data)));
            }

            ErrContainer<ErrType&> asErr() noexcept
                requires(std::is_reference_v<ErrType>) {
                return Err(std::get<1>(m_data).get());
            }

        public:
            /// @brief Returns true if the Result is Ok
            /// @return true if the Result is Ok
            bool isOk() const noexcept {
                return m_data.index() == 0;
            }

            /// @brief Returns true if the Result is Err
            /// @return true if the Result is Err
            bool isErr() const noexcept {
                return m_data.index() == 1;
            }

            /// @brief Unwraps the Ok value from the Result
            /// @throw std::runtime_error if the Result is Err
            /// @return the Ok value
            OkType unwrap() {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    throw std::runtime_error("Called unwrap on an error Result");
                }
            }

            /// @brief Unwraps the Err value from the Result
            /// @throw std::runtime_error if the Result is Ok
            /// @return the Err value
            ErrType unwrapErr() {
                if (isErr()) {
                    return std::move(std::get<1>(m_data));
                } else {
                    throw std::runtime_error("Called unwrapErr on an ok Result");
                }
            }

            /// @brief Returns a Result with references to the underlying values
            /// @return a Result with references to the underlying values
            Result<OkType&, ErrType&> asRef() noexcept;

            /// @brief Returns a Result with const references to the underlying values
            /// @return a Result with const references to the underlying values
            Result<OkType const&, ErrType const&> asConst() const noexcept;

            /// @brief Unwraps the Ok value from the Result, default constructing if unavailable
            /// @return the Ok value if available, otherwise a default constructed Ok value
            OkType unwrapOrDefault() noexcept(std::is_nothrow_default_constructible_v<OkType> && std::is_nothrow_move_constructible_v<OkType>)
                requires std::default_initializable<OkType> {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    return OkType();
                }
            }

            /// @brief Unwraps the Ok value from the Result, returning a default value if unavailable
            /// @param defaultValue the default value to return if the Result is Err
            /// @return the Ok value if available, otherwise the default value
            OkType unwrapOr(OkType&& defaultValue) noexcept(std::is_nothrow_move_constructible_v<OkType>) {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    return defaultValue;
                }
            }

            /// @brief Unwraps the Ok value from the Result, returning the result of an operation if unavailable
            /// @param operation the operation to perform if the Result is Err
            /// @return the Ok value if available, otherwise the result of the operation
            OkType unwrapOrElse(std::invocable auto&& operation) noexcept(std::is_nothrow_invocable_v<decltype(operation)>) {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    return operation();
                }
            }

            /// @brief Returns a Result with a copied Ok value
            /// @return a Result with a copied Ok value
            Result<std::remove_reference_t<OkType>, ErrType> copied() const
                noexcept(std::is_nothrow_copy_constructible_v<OkType>)
                requires(std::is_reference_v<OkType>);

        protected:
            void inspectInternal(std::invocable<OkType const&> auto&& operation) const noexcept(noexcept(operation(std::declval<OkType const&>()))) {
                if (this->isOk()) {
                    operation(std::get<0>(m_data));
                }
            }

            void inspectInternalErr(std::invocable<ErrType const&> auto&& operation) const noexcept(noexcept(operation(std::declval<ErrType const&>()))) {
                if (this->isErr()) {
                    operation(std::get<1>(m_data));
                }
            }
        };

        template <class ErrType>
        class ResultData<void, ErrType> {
        protected:
            using ProtectedErrType = std::conditional_t<std::is_reference_v<ErrType>, std::reference_wrapper<std::remove_reference_t<ErrType>>, ErrType>;
            std::variant<std::monostate, ProtectedErrType> m_data;

            template <std::size_t Index, class ClassType>
            requires std::constructible_from<ErrType, ClassType>
            ResultData(std::in_place_index_t<Index> index, ClassType&& err)
                noexcept(std::is_nothrow_constructible_v<ProtectedErrType, ClassType>)
                : m_data(index, std::move(err)) {}

            template <std::size_t Index>
            ResultData(std::in_place_index_t<Index> index) noexcept : m_data(index) {}

            ResultData(ResultData&& other) noexcept(std::is_nothrow_move_constructible_v<ProtectedErrType>)
                : m_data(std::move(other.m_data)) {}

            OkContainer<void> asOk() noexcept {
                return Ok();
            }

            ErrContainer<ErrType> asErr() noexcept
                requires(!std::is_reference_v<ErrType>) {
                return Err(std::move(std::get<1>(m_data)));
            }

            ErrContainer<ErrType&> asErr() noexcept
                requires(std::is_reference_v<ErrType>) {
                return Err(std::get<1>(m_data).get());
            }

        public:
            /// @brief Returns true if the Result is Ok
            /// @return true if the Result is Ok
            bool isOk() const noexcept {
                return m_data.index() == 0;
            }

            /// @brief Returns true if the Result is Err
            /// @return true if the Result is Err
            bool isErr() const noexcept {
                return m_data.index() == 1;
            }

            /// @brief Unwraps the Result
            /// @throw std::runtime_error if the Result is Err
            void unwrap() {
                if (isErr()) {
                    throw std::runtime_error("Called unwrap on an error Result");
                }
            }

            /// @brief Unwraps the Err value from the Result
            /// @throw std::runtime_error if the Result is Ok
            /// @return the Err value
            ErrType unwrapErr() {
                if (isErr()) {
                    return std::move(std::get<1>(m_data));
                } else {
                    throw std::runtime_error("Called unwrapErr on an ok Result");
                }
            }

            /// @brief Returns a Result with references to the underlying values
            /// @return a Result with references to the underlying values
            Result<void, ErrType&> asRef() noexcept;

            /// @brief Returns a Result with const references to the underlying values
            /// @return a Result with const references to the underlying values
            Result<void, ErrType const&> asConst() const noexcept;

            /// @brief Returns a Result with a copied Ok value
            /// @return a Result with a copied Ok value
            Result<void, ErrType> copied() const noexcept;

        protected:
            void inspectInternalErr(std::invocable<ErrType const&> auto&& operation) const noexcept(noexcept(operation(std::declval<ErrType const&>()))) {
                if (this->isErr()) {
                    operation(std::get<1>(m_data));
                }
            }
        };

        template <class OkType>
        class ResultData<OkType, void> {
        protected:
            using ProtectedOkType = std::conditional_t<std::is_reference_v<OkType>, std::reference_wrapper<std::remove_reference_t<OkType>>, OkType>;
            std::variant<ProtectedOkType, std::monostate> m_data;

            template <std::size_t Index, class ClassType>
            requires std::constructible_from<OkType, ClassType>
            ResultData(std::in_place_index_t<Index> index, ClassType&& ok)
                noexcept(std::is_nothrow_constructible_v<ProtectedOkType, ClassType>)
                : m_data(index, std::move(ok)) {}

            template <std::size_t Index>
            ResultData(std::in_place_index_t<Index> index) noexcept : m_data(index) {}

            ResultData(ResultData&& other) noexcept(std::is_nothrow_move_constructible_v<ProtectedOkType>)
                : m_data(std::move(other.m_data)) {}

            OkContainer<OkType> asOk() noexcept
                requires(!std::is_reference_v<OkType>) {
                return Ok(std::move(std::get<0>(m_data)));
            }

            OkContainer<OkType&> asOk() noexcept
                requires(std::is_reference_v<OkType>) {
                return Ok(std::get<0>(m_data).get());
            }

            ErrContainer<void> asErr() noexcept {
                return Err();
            }

        public:
            /// @brief Returns true if the Result is Ok
            /// @return true if the Result is Ok
            bool isOk() const noexcept {
                return m_data.index() == 0;
            }

            /// @brief Returns true if the Result is Err
            /// @return true if the Result is Err
            bool isErr() const noexcept {
                return m_data.index() == 1;
            }

            /// @brief Unwraps the Ok value from the Result
            /// @throw std::runtime_error if the Result is Err
            /// @return the Ok value
            OkType unwrap() {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    throw std::runtime_error("Called unwrap on an error Result");
                }
            }

            /// @brief Returns a Result with references to the underlying values
            /// @return a Result with references to the underlying values
            Result<OkType&, void> asRef() noexcept;

            /// @brief Returns a Result with const references to the underlying values
            /// @return a Result with const references to the underlying values
            Result<OkType const&, void> asConst() const noexcept;

            /// @brief Unwraps the Ok value from the Result, default constructing if unavailable
            /// @return the Ok value if available, otherwise a default constructed Ok value
            OkType unwrapOrDefault() noexcept(std::is_nothrow_default_constructible_v<OkType> && std::is_nothrow_move_constructible_v<OkType>)
                requires std::default_initializable<OkType> {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    return OkType();
                }
            }

            /// @brief Unwraps the Ok value from the Result, returning a default value if unavailable
            /// @param defaultValue the default value to return if the Result is Err
            /// @return the Ok value if available, otherwise the default value
            OkType unwrapOr(OkType&& defaultValue) noexcept(std::is_nothrow_move_constructible_v<OkType>) {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    return defaultValue;
                }
            }

            /// @brief Unwraps the Ok value from the Result, returning the result of an operation if unavailable
            /// @param operation the operation to perform if the Result is Err
            /// @return the Ok value if available, otherwise the result of the operation
            OkType unwrapOrElse(std::invocable auto&& operation) noexcept(std::is_nothrow_invocable_v<decltype(operation)>) {
                if (isOk()) {
                    return std::move(std::get<0>(m_data));
                } else {
                    return operation();
                }
            }

            /// @brief Returns a Result with a copied Ok value
            /// @return a Result with a copied Ok value
            Result<std::remove_reference_t<OkType>, void> copied() const
                noexcept(std::is_nothrow_copy_constructible_v<OkType>)
                requires(std::is_reference_v<OkType>);

        protected:
            void inspectInternal(std::invocable<OkType const&> auto&& operation) const
                noexcept(noexcept(operation(std::declval<OkType const&>()))) {
                if (this->isOk()) {
                    operation(std::get<0>(m_data));
                }
            }
        };

        template <>
        class ResultData<void, void> {
        protected:
            std::variant<std::monostate, std::monostate> m_data;

            template <std::size_t Index>
            inline ResultData(std::in_place_index_t<Index> index) noexcept : m_data(index) {}

            inline ResultData(ResultData&&) noexcept {}

            inline OkContainer<void> asOk() noexcept {
                return Ok();
            }

            inline ErrContainer<void> asErr() noexcept {
                return Err();
            }

        public:
            /// @brief Returns true if the Result is Ok
            /// @return true if the Result is Ok
            inline bool isOk() const noexcept {
                return m_data.index() == 0;
            }

            /// @brief Returns true if the Result is Err
            /// @return true if the Result is Err
            inline bool isErr() const noexcept {
                return m_data.index() == 1;
            }

            /// @brief Unwraps the Result
            /// @throw std::runtime_error if the Result is Err
            void unwrap() {
                if (isErr()) {
                    throw std::runtime_error("Called unwrap on an error Result");
                }
            }

            /// @brief Returns a Result with references to the underlying values
            /// @return a Result with references to the underlying values
            inline Result<void, void> asRef() noexcept;

            /// @brief Returns a Result with const references to the underlying values
            /// @return a Result with const references to the underlying values
            inline Result<void, void> asConst() const noexcept;

            /// @brief Returns a Result with a copied Ok value
            /// @return a Result with a copied Ok value
            inline Result<void, void> copied() const noexcept;
        };

        template <class OkType, class ErrType>
        class ResultDataWrapper : public ResultData<OkType, ErrType> {
        public:
            template <class OkType2>
            requires std::constructible_from<OkType, OkType2>
            ResultDataWrapper(OkContainer<OkType2>&& ok)
                noexcept(std::is_nothrow_constructible_v<ResultData<OkType, ErrType>, std::in_place_index_t<0>, OkType2>)
                : ResultData<OkType, ErrType>(std::in_place_index<0>, std::move(ok.unwrap())) {}

            template <class ErrType2>
            requires std::constructible_from<ErrType, ErrType2>
            ResultDataWrapper(ErrContainer<ErrType2>&& err)
                noexcept(std::is_nothrow_constructible_v<ResultData<OkType, ErrType>, std::in_place_index_t<1>, ErrType2>)
                : ResultData<OkType, ErrType>(std::in_place_index<1>, std::move(err.unwrap())) {}

            ResultDataWrapper(ResultDataWrapper&& other) noexcept(std::is_nothrow_move_constructible_v<ResultData<OkType, ErrType>>)
                : ResultData<OkType, ErrType>(std::move(other)) {}
        };

        template <class OkType>
        class ResultDataWrapper<OkType, void> : public ResultData<OkType, void> {
        public:
            template <class OkType2>
            requires std::constructible_from<OkType, OkType2>
            ResultDataWrapper(OkContainer<OkType2>&& ok)
                noexcept(std::is_nothrow_constructible_v<ResultData<OkType, void>, std::in_place_index_t<0>, OkType2>)
                : ResultData<OkType, void>(std::in_place_index<0>, std::move(ok.unwrap())) {}

            ResultDataWrapper(ErrContainer<void>&&)
                noexcept(std::is_nothrow_constructible_v<ResultData<OkType, void>, std::in_place_index_t<1>>)
                : ResultData<OkType, void>(std::in_place_index<1>) {}

            ResultDataWrapper(ResultDataWrapper&& other) noexcept(std::is_nothrow_move_constructible_v<ResultData<OkType, void>>)
                : ResultData<OkType, void>(std::move(other)) {}
        };

        template <class ErrType>
        class ResultDataWrapper<void, ErrType> : public ResultData<void, ErrType> {
        public:
            template <class ErrType2>
            requires std::constructible_from<ErrType, ErrType2>
            ResultDataWrapper(ErrContainer<ErrType2>&& err)
                noexcept(std::is_nothrow_constructible_v<ResultData<void, ErrType>, std::in_place_index_t<1>, ErrType2>)
                : ResultData<void, ErrType>(std::in_place_index<1>, std::move(err.unwrap())) {}

            ResultDataWrapper(OkContainer<void>&&)
                noexcept(std::is_nothrow_constructible_v<ResultData<void, ErrType>, std::in_place_index_t<0>>)
                : ResultData<void, ErrType>(std::in_place_index<0>) {}

            ResultDataWrapper(ResultDataWrapper&& other) noexcept(std::is_nothrow_move_constructible_v<ResultData<void, ErrType>>)
                : ResultData<void, ErrType>(std::move(other)) {}
        };

        template <>
        class ResultDataWrapper<void, void> : public ResultData<void, void> {
        public:
            inline ResultDataWrapper(OkContainer<void>&&)
                : ResultData<void, void>(std::in_place_index<0>) {}

            inline ResultDataWrapper(ErrContainer<void>&&)
                : ResultData<void, void>(std::in_place_index<1>) {}

            inline ResultDataWrapper(ResultDataWrapper&& other) noexcept(std::is_nothrow_move_constructible_v<ResultData<void, void>>)
                : ResultData<void, void>(std::move(other)) {}
        };
    }

    template <class OkType, class ErrType>
    class [[nodiscard]] Result final : public impl::ResultDataWrapper<OkType, ErrType> {
    public:
        using impl::ResultDataWrapper<OkType, ErrType>::ResultDataWrapper;

        Result() = delete;
        Result(Result const& other) = delete;
        Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<impl::ResultData<OkType, ErrType>>)
            : impl::ResultDataWrapper<OkType, ErrType>(std::move(other)) {}

        /// @brief Returns true if the Result is Ok
        /// @return true if the Result is Ok
        explicit operator bool() const noexcept {
            return this->isOk();
        }

        /// @brief Returns true if the Result is Ok and the Ok value satisfies the predicate
        /// @param predicate the predicate to check the Ok value against
        /// @return true if the Result is Ok and the Ok value satisfies the predicate
        bool isOkAnd(std::invocable<OkType> auto&& predicate) noexcept(noexcept(predicate(std::declval<OkType>()))) {
            return this->isOk() && predicate(std::forward<OkType>(this->unwrap()));
        }

        /// @brief Returns true if the Result is Err and the Err value satisfies the predicate
        /// @param predicate the predicate to check the Err value against
        /// @return true if the Result is Err and the Err value satisfies the predicate
        bool isErrAnd(std::invocable<ErrType> auto&& predicate) noexcept(noexcept(predicate(std::declval<ErrType>()))) {
            return this->isErr() && predicate(std::forward<ErrType>(this->unwrapErr()));
        }

        /// @brief Returns an std::optional containing the Ok value
        /// @return an std::optional containing the Ok value if the Result is Ok, otherwise std::nullopt
        std::optional<OkType> ok() noexcept {
            if (this->isOk()) {
                return this->unwrap();
            } else {
                return std::nullopt;
            }
        }

        /// @brief Returns an std::optional containing the Err value
        /// @return an std::optional containing the Err value if the Result is Err, otherwise std::nullopt
        std::optional<ErrType> err() noexcept {
            if (this->isErr()) {
                return this->unwrapErr();
            } else {
                return std::nullopt;
            }
        }

        /// @brief Maps the Ok value to a new Ok value using an operation
        /// @param operation the operation to map the Ok value with
        /// @return a new Result with the mapped Ok value if the Result is Ok, otherwise the Err value
        template <class Operation>
        requires std::invocable<Operation, OkType>
        Result<std::invoke_result_t<Operation, OkType>, ErrType> map(Operation&& operation)
            noexcept(noexcept(operation(std::declval<OkType>())))  {
            if (this->isOk()) {
                return Ok(operation(std::forward<OkType>(this->unwrap())));
            } else {
                return this->asErr();
            }
        }

        /// @brief Maps the Ok value to a new Ok value using an operation
        /// @param operation the operation to map the Ok value with
        /// @return a new Result with the mapped Ok value if the Result is Ok, otherwise the Err value
        template <class Operation>
        requires std::invocable<Operation>
        Result<std::invoke_result_t<Operation>, ErrType> map(Operation&& operation)
            noexcept(noexcept(operation())) {
            if (this->isOk()) {
                return Ok(operation());
            } else {
                return this->asErr();
            }
        }

        /// @brief Maps the Ok value to a new value using an operation
        /// @param defaultValue the default value to return if the Result is Err
        /// @param operation the operation to map the Ok value with
        /// @return the mapped Ok value if the Result is Ok, otherwise the default value
        template <class OkType2, class Operation>
        requires (std::invocable<Operation, OkType> && std::convertible_to<std::invoke_result_t<Operation, OkType>, OkType2>)
        OkType2 mapOr(OkType2&& defaultValue, Operation&& operation)
            noexcept(noexcept(operation(std::declval<OkType>()))) {
            if (this->isOk()) {
                return operation(std::forward<OkType>(this->unwrap()));
            } else {
                return std::move(defaultValue);
            }
        }

        /// @brief Maps the Ok value to a new value using an operation
        /// @param defaultValue the default value to return if the Result is Err
        /// @param operation the operation to map the Ok value with
        /// @return the mapped Ok value if the Result is Ok, otherwise the default value
        template <class OkType2, class Operation>
        requires (std::invocable<Operation> && std::convertible_to<std::invoke_result_t<Operation>, OkType2>)
        OkType2 mapOr(OkType2&& defaultValue, Operation&& operation)
            noexcept(noexcept(operation())) {
            if (this->isOk()) {
                return operation();
            } else {
                return std::move(defaultValue);
            }
        }

        /// @brief Maps the Ok value to a new value using an operation
        /// @param defaultValue the operation to perform if the Result is Err
        /// @param operation the operation to map the Ok value with
        /// @return the mapped Ok value if the Result is Ok, otherwise the result of the operation
        template <class DefaultValue, class Operation>
        requires (std::invocable<DefaultValue> && std::invocable<Operation, OkType>
            && std::convertible_to<std::invoke_result_t<DefaultValue>, std::invoke_result_t<Operation, OkType>>)
        std::invoke_result_t<Operation, OkType> mapOrElse(DefaultValue&& defaultValue, Operation&& operation)
            noexcept(noexcept(operation(std::declval<OkType>())) && noexcept(defaultValue())) {
            if (this->isOk()) {
                return operation(std::forward<OkType>(this->unwrap()));
            } else {
                return defaultValue();
            }
        }

        /// @brief Maps the Ok value to a new value using an operation
        /// @param defaultValue the operation to perform if the Result is Err
        /// @param operation the operation to map the Ok value with
        /// @return the mapped Ok value if the Result is Ok, otherwise the result of the operation
        template <class DefaultValue, class Operation>
        requires (std::invocable<DefaultValue> && std::invocable<Operation>
            && std::convertible_to<std::invoke_result_t<DefaultValue>, std::invoke_result_t<Operation>>)
        std::invoke_result_t<Operation> mapOrElse(DefaultValue&& defaultValue, Operation&& operation)
            noexcept(noexcept(operation()) && noexcept(defaultValue())) {
            if (this->isOk()) {
                return operation();
            } else {
                return defaultValue();
            }
        }


        /// @brief Maps the Err value to a new Err value using an operation
        /// @param operation the operation to map the Err value with
        /// @return a new Result with the Ok value if the Result is Ok, otherwise the mapped Err value
        template <class Operation>
        requires std::invocable<Operation, ErrType>
        Result<OkType, std::invoke_result_t<Operation, ErrType>> mapErr(Operation&& operation)
            noexcept(noexcept(operation(std::declval<ErrType>()))) {
            if (this->isOk()) {
                return this->asOk();
            } else {
                return Err(operation(std::forward<ErrType>(this->unwrapErr())));
            }
        }

        /// @brief Maps the Err value to a new Err value using an operation
        /// @param operation the operation to map the Err value with
        /// @return a new Result with the Ok value if the Result is Ok, otherwise the mapped Err value
        template <class Operation>
        requires std::invocable<Operation>
        Result<OkType, std::invoke_result_t<Operation>> mapErr(Operation&& operation)
            noexcept(noexcept(operation())) {
            if (this->isOk()) {
                return this->asOk();
            } else {
                return Err(operation());
            }
        }

        /// @brief Inspects the Ok value with an operation
        /// @param operation the operation to call the Ok value with
        /// @return the Result itself
        Result<OkType, ErrType>& inspect(std::invocable<OkType const&> auto&& operation)
            noexcept(noexcept(operation(std::declval<OkType const&>()))) {
            this->inspectInternal(operation);
            return *this;
        }

        /// @brief Inspects the Err value with an operation
        /// @param operation the operation to call the Err value with
        /// @return the Result itself
        Result<OkType, ErrType>& inspectErr(std::invocable<ErrType const&> auto&& operation)
            noexcept(noexcept(operation(std::declval<ErrType const&>()))) {
            this->inspectInternalErr(operation);
            return *this;
        }

        /// @brief Returns the other Result if this Result is Ok, otherwise returns this Result
        /// @param other the other Result to return if this Result is Ok
        /// @return the other Result if this Result is Ok, otherwise this Result
        template <class OkType2>
        Result<OkType2, ErrType> and_(Result<OkType2, ErrType>&& other) noexcept {
            if (this->isOk()) {
                return std::move(other);
            } else {
                return this->asErr();
            }
        }

        /// @brief Returns the result of an operation if this Result is Ok, otherwise returns this Result
        /// @param operation the operation to perform if this Result is Ok
        /// @return the result of the operation if this Result is Ok, otherwise this Result
        template <class Operation>
        requires (std::invocable<Operation, OkType> && impl::IsResult<std::invoke_result_t<Operation, OkType>>
            && std::convertible_to<ErrType, impl::ResultErrType<std::invoke_result_t<Operation, OkType>>>)
        std::invoke_result_t<Operation, OkType> andThen(Operation&& operation) noexcept(noexcept(operation(std::declval<OkType>()))) {
            if (this->isOk()) {
                return operation(std::forward<OkType>(this->unwrap()));
            } else {
                return this->asErr();
            }
        }

        /// @brief Returns the result of an operation if this Result is Ok, otherwise returns this Result
        /// @param operation the operation to perform if this Result is Ok
        /// @return the result of the operation if this Result is Ok, otherwise this Result
        template <class Operation>
        requires (std::invocable<Operation> && impl::IsResult<std::invoke_result_t<Operation>>
            && std::convertible_to<ErrType, impl::ResultErrType<std::invoke_result_t<Operation>>>)
        std::invoke_result_t<Operation> andThen(Operation&& operation) noexcept(noexcept(operation())) {
            if (this->isOk()) {
                return operation();
            } else {
                return this->asErr();
            }
        }

        /// @brief Returns the other Result if this Result is Err, otherwise returns this Result
        /// @param other the other Result to return if this Result is Err
        /// @return the other Result if this Result is Err, otherwise this Result
        template <class ErrType2>
        Result<OkType, ErrType2> or_(Result<OkType, ErrType2>&& other) noexcept {
            if (this->isOk()) {
                return this->asOk();
            } else {
                return std::move(other);
            }
        }

        /// @brief Returns the result of an operation if this Result is Err, otherwise returns this Result
        /// @param operation the operation to perform if this Result is Err
        /// @return the result of the operation if this Result is Err, otherwise this Result
        template <class Operation>
        requires (std::invocable<Operation, ErrType> && impl::IsResult<std::invoke_result_t<Operation, ErrType>>
            && std::convertible_to<OkType, impl::ResultOkType<std::invoke_result_t<Operation, ErrType>>>)
        std::invoke_result_t<Operation, ErrType> orElse(Operation&& operation) noexcept(noexcept(operation(std::declval<ErrType>()))) {
            if (this->isOk()) {
                return this->asOk();
            } else {
                return operation(std::forward<ErrType>(this->unwrapErr()));
            }
        }

        /// @brief Returns the result of an operation if this Result is Err, otherwise returns this Result
        /// @param operation the operation to perform if this Result is Err
        /// @return the result of the operation if this Result is Err, otherwise this Result
        template <class Operation>
        requires (std::invocable<Operation> && impl::IsResult<std::invoke_result_t<Operation>>
            && std::convertible_to<OkType, impl::ResultOkType<std::invoke_result_t<Operation>>>)
        std::invoke_result_t<Operation> orElse(Operation&& operation) noexcept(noexcept(operation())) {
            if (this->isOk()) {
                return this->asOk();
            } else {
                return operation();
            }
        }

        /// @brief Transposes the Result from Result<std::optional<OkType>, ErrType> to std::optional<Result<OkType, ErrType>>
        /// @return std::nullopt if Ok value is empty, otherwise the Result wrapped in an optional
        std::optional<Result<impl::OptionalType<OkType>, ErrType>> transpose()
            noexcept(std::is_nothrow_move_constructible_v<OkType> && std::is_nothrow_move_constructible_v<ErrType>)
            requires(impl::IsOptional<OkType>) {
            if (this->isOk()) {
                auto res = this->unwrap();
                if (res.has_value()) {
                    return Ok(std::move(res.value()));
                } else {
                    return std::nullopt;
                }
            } else {
                return this->asErr();
            }
        }

        /// @brief Flattens the Result from Result<Result<OkType, ErrType>, ErrType> to Result<OkType, ErrType>
        /// @return the inner Result if the Result is Ok, otherwise the outer Result
        Result<impl::ResultOkType<OkType>, ErrType> flatten()
            noexcept(std::is_nothrow_move_constructible_v<OkType> && std::is_nothrow_move_constructible_v<ErrType>)
            requires(impl::IsResult<OkType> && std::same_as<ErrType, impl::ResultErrType<OkType>>) {
            if (this->isOk()) {
                return this->unwrap();
            } else {
                return this->asErr();
            }
        }
    };

    namespace impl {
        template <class OkType, class ErrType>
        Result<OkType&, ErrType&> ResultData<OkType, ErrType>::asRef() noexcept {
            if (this->isOk()) {
                return Ok(std::get<0>(m_data));
            } else {
                return Err(std::get<1>(m_data));
            }
        }

        template <class ErrType>
        Result<void, ErrType&> ResultData<void, ErrType>::asRef() noexcept {
            if (this->isOk()) {
                return Ok();
            } else {
                return Err(std::get<1>(m_data));
            }
        }

        template <class OkType>
        Result<OkType&, void> ResultData<OkType, void>::asRef() noexcept {
            if (this->isOk()) {
                return Ok(std::get<0>(m_data));
            } else {
                return Err();
            }
        }

        Result<void, void> ResultData<void, void>::asRef() noexcept {
            if (this->isOk()) {
                return Ok();
            } else {
                return Err();
            }
        }

        template <class OkType, class ErrType>
        Result<OkType const&, ErrType const&> ResultData<OkType, ErrType>::asConst() const noexcept {
            if (this->isOk()) {
                return Ok(std::get<0>(m_data));
            } else {
                return Err(std::get<1>(m_data));
            }
        }

        template <class ErrType>
        Result<void, ErrType const&> ResultData<void, ErrType>::asConst() const noexcept {
            if (this->isOk()) {
                return Ok();
            } else {
                return Err(std::get<1>(m_data));
            }
        }

        template <class OkType>
        Result<OkType const&, void> ResultData<OkType, void>::asConst() const noexcept {
            if (this->isOk()) {
                return Ok(std::get<0>(m_data));
            } else {
                return Err();
            }
        }

        Result<void, void> ResultData<void, void>::asConst() const noexcept {
            if (this->isOk()) {
                return Ok();
            } else {
                return Err();
            }
        }

        template <class OkType, class ErrType>
        Result<std::remove_reference_t<OkType>, ErrType> ResultData<OkType, ErrType>::copied() const
            noexcept(std::is_nothrow_copy_constructible_v<OkType>)
            requires(std::is_reference_v<OkType>) {
            if (this->isOk()) {
                return Ok(std::get<0>(m_data).get());
            } else {
                return Err(std::get<1>(m_data));
            }
        }

        template <class ErrType>
        Result<void, ErrType> ResultData<void, ErrType>::copied() const noexcept {
            if (this->isOk()) {
                return Ok();
            } else {
                return Err(std::get<1>(m_data));
            }
        }

        template <class OkType>
        Result<std::remove_reference_t<OkType>, void> ResultData<OkType, void>::copied() const
            noexcept(std::is_nothrow_copy_constructible_v<OkType>)
            requires(std::is_reference_v<OkType>) {
            if (this->isOk()) {
                return Ok(std::get<0>(m_data).get());
            } else {
                return Err();
            }
        }

        Result<void, void> ResultData<void, void>::copied() const noexcept {
            if (this->isOk()) {
                return Ok();
            } else {
                return Err();
            }
        }
    }
}
