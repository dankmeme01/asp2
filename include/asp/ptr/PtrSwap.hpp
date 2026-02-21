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
            auto block = unpackPtr(m_block.load(std::memory_order::relaxed));
            if (block) block->releaseStrong();
            m_block.store(other.m_block.exchange(0, std::memory_order::acq_rel), std::memory_order::relaxed);
        }
        return *this;
    }

    ~PtrSwap() {
        auto block = unpackPtr(m_block.load(std::memory_order::relaxed));
        if (block) block->releaseStrong();
    }

    Ptr load() const {
        while (true) {
            auto raw = m_block.load(std::memory_order::acquire);
            auto block = unpackPtr(raw);
            if (!block) {
                return Ptr{};
            }

            block->retainStrong();

            // check if the block is still the same, compare including the tag
            if (raw == m_block.load(std::memory_order::acquire)) {
                return Ptr::adoptFromRaw(block);
            }

            // changed
            block->releaseStrong();
        }
    }

    void store(const Ptr& ptr) {
        this->swap(ptr);
    }

    void store(Ptr&& ptr) {
        this->swap(std::move(ptr));
    }

    Ptr swap(const Ptr& ptr) {
        uintptr_t oldRaw = m_block.load(std::memory_order::acquire);
        Block* newBlock = ptr.m_block;

        if (ptr.m_block) ptr.m_block->retainStrong();

        while (true) {
            if (m_block.compare_exchange_weak(oldRaw, 
                pack(newBlock, nextAba(unpackAba(oldRaw))), std::memory_order::acq_rel, std::memory_order::acquire
            )) {
                return Ptr::adoptFromRaw(unpackPtr(oldRaw));
            }
        }
    }

    Ptr swap(Ptr&& ptr) {
        uintptr_t oldRaw = m_block.load(std::memory_order::acquire);
        Block* newBlock = ptr.m_block;

        while (true) {
            if (m_block.compare_exchange_weak(oldRaw, 
                pack(newBlock, nextAba(unpackAba(oldRaw))), std::memory_order::acq_rel, std::memory_order::acquire
            )) {
                // skip the retain and just null out the moved-from ptr
                ptr.leak();

                return Ptr::adoptFromRaw(unpackPtr(oldRaw));
            }
        }
    }

    /// Performs an atomic Read-Copy-Update operation. The provided function may be called multiple times, with the current value (const Ptr&),
    /// and is expected to return a new SharedPtr to store.
    Ptr rcu(auto&& f) noexcept requires (std::is_invocable_r_v<Ptr, decltype(f), const Ptr&>) {
        while (true) {
            uintptr_t oldRaw = m_block.load(std::memory_order::acquire);
            Block* oldBlock = unpackPtr(oldRaw);

            Ptr oldPtr = Ptr::adoptFromRaw(oldBlock);
            if (oldBlock) oldBlock->retainStrong();

            Ptr newPtr = f(oldPtr);
            Block* newBlock = newPtr.m_block;
            uintptr_t newRaw = pack(newBlock, nextAba(unpackAba(oldRaw)));

            // early return if the block is the same, no need to swap
            if (oldBlock == newBlock) {
                return newPtr;
            }

            if (newBlock) newBlock->retainStrong();

            if (m_block.compare_exchange_weak(oldRaw, newRaw, std::memory_order::acq_rel, std::memory_order::acquire)) {
                // need to release the old ptr twice (once for the load and once for the swap)
                if (oldBlock) oldBlock->releaseStrong();

                return newPtr;
            }

            if (newBlock) newBlock->releaseStrong();
        }
    }

private:
    // use highest 8 bits for aba in 64 bit platforms
    // or use lower 4 bits for aba in 32 bit platforms
    std::atomic<uintptr_t> m_block{0};

    static constexpr uintptr_t AbaMask = (sizeof(uintptr_t) == 8) ? (0xFFull << 56) : 0xFul;
    static constexpr uintptr_t AbaShift = (sizeof(uintptr_t) == 8) ? 56 : 0;
    static constexpr uintptr_t PtrMask = ~AbaMask;

    static uintptr_t pack(Block* ptr, uintptr_t aba) noexcept {
        return (reinterpret_cast<uintptr_t>(ptr) & PtrMask) | ((aba << AbaShift) & AbaMask);
    }

    static Block* unpackPtr(uintptr_t packed) noexcept {
        return reinterpret_cast<Block*>(packed & PtrMask);
    }

    static uintptr_t unpackAba(uintptr_t packed) noexcept {
        return (packed & AbaMask) >> AbaShift;
    }

    static uintptr_t nextAba(uintptr_t aba) noexcept {
        return (aba + 1) & (AbaMask >> AbaShift);
    }
};

}