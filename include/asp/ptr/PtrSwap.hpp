#pragma once
#include "SharedPtr.hpp"

namespace asp {

template <typename T>
class PtrSwap {
public:
    using Ptr = SharedPtr<T>;
    using Block = SharedPtrBlock<T>;

    PtrSwap() {}
    PtrSwap(std::nullptr_t) : PtrSwap() {}

    PtrSwap(const SharedPtr<T>& ptr) {
        this->store(ptr);
    }

    PtrSwap(const PtrSwap&) = delete;
    PtrSwap& operator=(const PtrSwap&) = delete;
    PtrSwap(PtrSwap&& other) noexcept : m_block(other.m_block.load(std::memory_order::relaxed)) {}

    PtrSwap& operator=(PtrSwap&& other) noexcept {
        if (this != &other) {
            this->release();
            m_block.store(other.m_block.load(std::memory_order::relaxed), std::memory_order::relaxed);
        }
        return *this;
    }

    ~PtrSwap() {
        this->release();
    }

    Ptr load() const {
        return Ptr{this->block<true>()};
    }

    void store(const Ptr& ptr) {
        this->swap(ptr);
    }

    void store(Ptr&& ptr) {
        this->swap(std::move(ptr));
    }

    Ptr swap(const Ptr& ptr) {
        size_t newB = reinterpret_cast<size_t>(ptr.m_block);
        size_t oldB = m_block.exchange(newB, std::memory_order::seq_cst);
        this->retain(ptr);

        return Ptr::adoptFromRaw(reinterpret_cast<Block*>(oldB));
    }

    Ptr swap(Ptr&& ptr) {
        size_t newB = reinterpret_cast<size_t>(ptr.m_block);
        size_t oldB = m_block.exchange(newB, std::memory_order::seq_cst);

        // skip the retain and just null out the moved-from ptr
        ptr.leak();

        return Ptr::adoptFromRaw(reinterpret_cast<Block*>(oldB));
    }

private:
    std::atomic<size_t> m_block{0};

    template <bool Acquire = true>
    Block* block() const {
        return reinterpret_cast<Block*>(m_block.load(
            Acquire ? std::memory_order::acquire : std::memory_order::relaxed
        ));
    }

    void release() {
        // we just let SharedPtr handle this
        auto _ = Ptr::adoptFromRaw(this->block<false>());
    }

    void retain(const Ptr& ptr) {
        if (ptr.m_block) {
            ptr.m_block->strong.fetch_add(1, std::memory_order::relaxed);
        }
    }
};

}