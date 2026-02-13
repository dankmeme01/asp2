#pragma once
#include <asp/ptr/SharedPtr.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>
#include <string_view>
#include <fmt/format.h>

namespace asp {

template <typename Derived, typename Base>
struct BoxedStringImpl;

struct BoxedStringImplUnique {
    BoxedStringImplUnique() = default;
    BoxedStringImplUnique(std::string_view str) {
        if (str.empty()) return;

        m_size = str.size();
        m_data = std::make_unique<char[]>(m_size + 1);
        std::copy(str.begin(), str.end(), m_data.get());
        m_data[m_size] = '\0';
    }

    BoxedStringImplUnique(BoxedStringImplUnique&&) = default;
    BoxedStringImplUnique& operator=(BoxedStringImplUnique&&) = default;
    BoxedStringImplUnique(const BoxedStringImplUnique&) = delete;
    BoxedStringImplUnique& operator=(const BoxedStringImplUnique&) = delete;

    std::string_view view() const noexcept {
        return std::string_view(m_data.get(), m_size);
    }

    const char* c_str() const noexcept {
        return m_data ? m_data.get() : "";
    }

private:
    template <typename D, typename B>
    friend struct BoxedStringImpl;

    std::unique_ptr<char[]> m_data;
    size_t m_size = 0;

    BoxedStringImplUnique(size_t size) {
        if (!size) return;

        m_size = size;
        m_data = std::make_unique<char[]>(m_size + 1);
        m_data[m_size] = '\0';
    }
};

struct BoxedStringImplShared {
    BoxedStringImplShared() = default;
    BoxedStringImplShared(std::string_view str) {
        if (str.empty()) return;

        auto size = str.size();
        m_data = asp::make_shared<char[]>(size + 1);
        std::copy(str.begin(), str.end(), m_data.get());
        m_data[size] = '\0';
    }

    std::string_view view() const noexcept {
        if (m_data) {
            return std::string_view(m_data.get(), m_data.size() - 1);
        } else {
            return std::string_view();
        }
    }

    const char* c_str() const noexcept {
        return m_data ? m_data.get() : "";
    }

    BoxedStringImplShared(BoxedStringImplShared&&) = default;
    BoxedStringImplShared& operator=(BoxedStringImplShared&&) = default;
    BoxedStringImplShared(const BoxedStringImplShared&) = default;
    BoxedStringImplShared& operator=(const BoxedStringImplShared&) = default;

private:
    template <typename D, typename B>
    friend struct BoxedStringImpl;

    asp::SharedPtr<char[]> m_data;

    BoxedStringImplShared(size_t size) {
        if (!size) return;

        m_data = asp::make_shared<char[]>(size + 1);
        m_data.get()[size] = '\0';
    }
};

template <typename Derived, typename Base>
struct BoxedStringImpl : Base {
    using Base::Base;
    // using Base::view;
    // using Base::c_str;
    // using Base::size;

    BoxedStringImpl(const std::string& str) : Base(std::string_view(str)) {}
    BoxedStringImpl(const char* str) : Base(std::string_view(str)) {}

    operator std::string_view() const noexcept {
        return this->view();
    }

    size_t size() const noexcept {
        return this->view().size();
    }

    bool empty() const noexcept {
        return this->size() == 0;
    }

    bool starts_with(std::string_view prefix) const noexcept {
        return this->view().starts_with(prefix);
    }

    bool ends_with(std::string_view suffix) const noexcept {
        return this->view().ends_with(suffix);
    }

    bool contains(std::string_view substr) const noexcept {
        return this->view().find(substr) != std::string_view::npos;
    }

    std::string_view substr(size_t pos, size_t count = std::string_view::npos) const noexcept {
        return this->view().substr(pos, count);
    }

    bool operator==(std::string_view other) const noexcept {
        return this->view() == other;
    }

    template <typename... Args>
    static Derived format(fmt::format_string<Args...> fmt, Args&&... args) {
        size_t size = fmt::formatted_size(fmt, std::forward<Args>(args)...);
        Derived result(size);
        char* ptr = result.m_data.get();

        auto formattedSize = fmt::format_to_n(ptr, size, fmt, std::forward<Args>(args)...).size;
        if (formattedSize < size) {
            ptr[formattedSize] = '\0';
        } else {
            ptr[size] = '\0';
        }

        return result;
    }
};

template <typename D, typename B>
auto format_as(const BoxedStringImpl<D, B>& s) -> std::string_view {
    return s.view();
}

struct BoxedString : BoxedStringImpl<BoxedString, BoxedStringImplShared> {
    using BoxedStringImpl::BoxedStringImpl;
};

struct UniqueBoxedString : BoxedStringImpl<UniqueBoxedString, BoxedStringImplUnique> {
    using BoxedStringImpl::BoxedStringImpl;

    UniqueBoxedString clone() const {
        return UniqueBoxedString(this->view());
    }
};

}