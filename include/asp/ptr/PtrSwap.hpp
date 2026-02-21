#pragma once
#include "SharedPtr.hpp"
#include <asp/detail/config.hpp>

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
    PtrSwap(PtrSwap&& other) noexcept {
        m_block.store(other.m_block.exchange(0, std::memory_order::acq_rel), std::memory_order::relaxed);
    }

    PtrSwap& operator=(PtrSwap&& other) noexcept {
        if (this != &other) {
            auto block = reinterpret_cast<Block*>(m_block.load(std::memory_order::relaxed));
            if (block) block->releaseStrong();
            m_block.store(other.m_block.exchange(0, std::memory_order::acq_rel), std::memory_order::relaxed);
        }
        return *this;
    }

    ~PtrSwap() {
        auto block = reinterpret_cast<Block*>(m_block.load(std::memory_order::relaxed));
        if (block) block->releaseStrong();
    }

    Ptr load() const {
        while (true) {
            auto block = reinterpret_cast<Block*>(m_block.load(std::memory_order::acquire));
            if (!block) {
                return Ptr{};
            }

            block->retainStrong();

            // check if the block is still the same
            if (block == reinterpret_cast<Block*>(m_block.load(std::memory_order::acquire))) {
                return Ptr::adoptFromRaw(block);
            }

            // changed
            block->releaseStrong();
        }
        std::unreachable();
    }

    void store(const Ptr& ptr) {
        this->swap(ptr);
    }

    void store(Ptr&& ptr) {
        this->swap(std::move(ptr));
    }

    Ptr swap(const Ptr& ptr) {
        size_t newB = reinterpret_cast<size_t>(ptr.m_block);
        if (ptr.m_block) ptr.m_block->retainStrong();
        
        size_t oldB = m_block.exchange(newB, std::memory_order::acq_rel);

        return Ptr::adoptFromRaw(reinterpret_cast<Block*>(oldB));
    }

    Ptr swap(Ptr&& ptr) {
        size_t newB = reinterpret_cast<size_t>(ptr.m_block);
        size_t oldB = m_block.exchange(newB, std::memory_order::acq_rel);

        // skip the retain and just null out the moved-from ptr
        ptr.leak();

        return Ptr::adoptFromRaw(reinterpret_cast<Block*>(oldB));
    }

    /// Performs an atomic Read-Copy-Update operation. The provided function may be called multiple times, with the current value (const Ptr&),
    /// and is expected to return a new SharedPtr to store.
    Ptr rcu(auto&& f) noexcept requires (std::is_invocable_r_v<Ptr, decltype(f), const Ptr&>) {
        Ptr oldPtr = this->load();

        while (true) {
            auto newPtr = f(oldPtr);

            auto oldBlock = reinterpret_cast<size_t>(oldPtr.m_block);
            auto newBlock = reinterpret_cast<size_t>(newPtr.m_block);

            if (newBlock) reinterpret_cast<Block*>(newBlock)->retainStrong();

            if (m_block.compare_exchange_weak(oldBlock, newBlock, std::memory_order::acq_rel, std::memory_order::acquire)) {
                // need to release the old ptr twice (once for the load and once for the swap)
                if (oldBlock) reinterpret_cast<Block*>(oldBlock)->releaseStrong();

                return newPtr;
            }
            if (newBlock) reinterpret_cast<Block*>(newBlock)->releaseStrong();

            // cas fail, reload
            oldPtr = this->load();
        }
    }

private:
    // use highest 
    std::atomic<size_t> m_block{0};
};

}