#pragma once
#include <asp/detail/config.hpp>
#include <atomic>
#include <utility>
#include <memory>

namespace asp {

template <typename T>
class WeakPtr;
template <typename T>
class SharedPtr;
template <typename T>
class PtrSwap;

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args&&... args);

using SharedPtrDtor = void(*)(void*);

struct SharedPtrBlockBase {
    std::atomic<size_t> strong;
    std::atomic<size_t> weak;
    SharedPtrDtor dtor;
};

template <typename T>
struct SharedPtrBlock : SharedPtrBlockBase {
    T data;

    template <typename... Args>
    static SharedPtrBlock* create(Args&&... args) {
        // wrap into unique_ptr for exception safety
        std::unique_ptr<SharedPtrBlock, void(*)(void*)> mem{
            reinterpret_cast<SharedPtrBlock*>(::operator new(sizeof(SharedPtrBlock))),
            +[](void* ptr) { ::operator delete(ptr); }
        };

        mem->strong.store(1);
        mem->weak.store(1);
        mem->dtor = [](void* ptr) {
            auto block = reinterpret_cast<SharedPtrBlock*>(ptr);
            block->data.~T();
        };
        new (&mem->data) T(std::forward<Args>(args)...);

        return mem.release();
    }

    T* ptr() noexcept {
        return &data;
    }
};

template <typename T>
struct SharedPtrBlock<T[]> : SharedPtrBlockBase {
    size_t size;
    T data[];

    static SharedPtrBlock* create(size_t size) {
        // wrap into unique_ptr for exception safety
        std::unique_ptr<SharedPtrBlock, void(*)(void*)> mem{
            reinterpret_cast<SharedPtrBlock*>(::operator new(sizeof(SharedPtrBlock) + sizeof(T) * size)),
            +[](void* ptr) { ::operator delete(ptr); }
        };

        mem->strong.store(1);
        mem->weak.store(1);
        mem->dtor = [](void* ptr) {
            auto block = reinterpret_cast<SharedPtrBlock*>(ptr);
            for (size_t i = 0; i < block->size; i++) {
                block->data[i].~T();
            }
        };
        mem->size = size;

        // default initialize `size` elements
        for (size_t i = 0; i < size; i++) {
            new (&mem->data[i]) T();
        }

        return mem.release();
    }

    T* ptr() noexcept {
        return &data[0];
    }
};

template <typename T>
class SharedPtr {
public:
    SharedPtr() : m_block(nullptr) {}
    SharedPtr(std::nullptr_t) : m_block(nullptr) {}

    static SharedPtr adoptFromRaw(SharedPtrBlock<T>* block) {
        SharedPtr ptr;
        ptr.m_block = block;
        return ptr;
    }

    SharedPtr(SharedPtrBlock<T>* block) : m_block(block) {
        if (m_block) m_block->strong.fetch_add(1, std::memory_order::relaxed);
    }

    SharedPtr(const SharedPtr& other) : SharedPtr(other.m_block) {}

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            this->release();
            m_block = other.m_block;
            if (m_block) m_block->strong.fetch_add(1, std::memory_order::relaxed);
        }
        return *this;
    }

    SharedPtr(SharedPtr&& other) noexcept : m_block(std::exchange(other.m_block, nullptr)) {}

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            this->release();
            m_block = std::exchange(other.m_block, nullptr);
        }
        return *this;
    }

    ~SharedPtr() {
        this->release();
    }

    size_t strongCount() const noexcept {
        return m_block ? m_block->strong.load(std::memory_order::relaxed) : 0;
    }

    size_t weakCount() const noexcept {
        return m_block ? m_block->weak.load(std::memory_order::relaxed) : 0;
    }

    decltype(auto) get() const noexcept {
        return m_block ? m_block->ptr() : nullptr;
    }

    size_t size() const noexcept requires (std::is_array_v<T>) {
        return m_block ? m_block->size : 0;
    }

    decltype(auto) operator*() const noexcept {
        return *m_block->ptr();
    }

    decltype(auto) operator[](size_t index) const noexcept requires std::is_array_v<T> {
        return m_block->ptr()[index];
    }

    decltype(auto) operator->() const noexcept {
        return m_block->ptr();
    }

    operator bool() const noexcept {
        return m_block != nullptr;
    }

    bool operator==(std::nullptr_t) const noexcept {
        return m_block == nullptr;
    }

    bool operator==(const SharedPtr& other) const noexcept {
        return m_block == other.m_block;
    }

    void leak() noexcept {
        m_block = nullptr;
    }

    void reset() {
        this->release();
        m_block = nullptr;
    }

    template <typename Y>
    operator SharedPtr<Y>() const requires std::is_convertible_v<T*, Y*> {
        SharedPtrBlockBase* base = m_block;
        return SharedPtr<Y>{reinterpret_cast<SharedPtrBlock<Y>*>(base)};
    }

private:
    friend class WeakPtr<T>;
    friend class PtrSwap<T>;

    template <typename U, typename... Args>
    friend SharedPtr<U> make_shared(Args&&... args);

    SharedPtrBlock<T>* m_block;

    void release();
    void destroyData();
    void _initSharedFromThis(T* ptr);
};

template <typename T>
class WeakPtr {
public:
    WeakPtr() : m_block(nullptr) {}
    WeakPtr(std::nullptr_t) : m_block(nullptr) {}

    static WeakPtr adoptFromRaw(SharedPtrBlock<T>* block) {
        WeakPtr ptr;
        ptr.m_block = block;
        return ptr;
    }

    WeakPtr(const SharedPtr<T>& shared) : m_block(shared.m_block) {
        if (m_block) m_block->weak.fetch_add(1, std::memory_order::relaxed);
    }

    WeakPtr(const WeakPtr& other) : m_block(other.m_block) {
        if (m_block) m_block->weak.fetch_add(1, std::memory_order::relaxed);
    }

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            this->release();
            m_block = other.m_block;
            if (m_block) m_block->weak.fetch_add(1, std::memory_order::relaxed);
        }
        return *this;
    }

    WeakPtr(WeakPtr&& other) noexcept : m_block(std::exchange(other.m_block, nullptr)) {}

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            this->release();
            m_block = std::exchange(other.m_block, nullptr);
        }
        return *this;
    }

    ~WeakPtr() {
        this->release();
    }

    SharedPtr<T> upgrade() const {
        if (!m_block) return SharedPtr<T>();

        size_t strong = m_block->strong.load(std::memory_order::relaxed);
        while (strong != 0) {
            if (m_block->strong.compare_exchange_weak(strong, strong + 1, std::memory_order::acquire, std::memory_order::relaxed)) {
                return SharedPtr<T>::adoptFromRaw(reinterpret_cast<SharedPtrBlock<T>*>(m_block));
            }
        }

        return SharedPtr<T>();
    }

    bool expired() const {
        return !m_block || m_block->strong.load(std::memory_order::relaxed) == 0;
    }

private:
    SharedPtrBlockBase* m_block;

    void release();
    void destroyBlock();
};

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args&&... args) {
    auto block = SharedPtrBlock<T>::create(std::forward<Args>(args)...);
    auto sp = SharedPtr<T>::adoptFromRaw(block);
    if constexpr (!std::is_array_v<T>) {
        sp._initSharedFromThis(sp.get());
    }
    return sp;
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return make_shared<T>(std::forward<Args>(args)...);
}


template <typename T>
void SharedPtr<T>::release() {
    if (!m_block) return;

    if (m_block->strong.fetch_sub(1, std::memory_order::release) == 1) [[unlikely]] {
        this->destroyData();
    }
}

template <typename T>
ASP_COLD void SharedPtr<T>::destroyData() {
    std::atomic_thread_fence(std::memory_order::acquire);

    auto weak = WeakPtr<T>::adoptFromRaw(m_block);
    m_block->dtor(m_block);

    // dtor of weak will handle the actual deallocation, if necessary
}

template <typename T>
void WeakPtr<T>::release() {
    if (!m_block) return;

    if (m_block->weak.fetch_sub(1, std::memory_order::release) == 1) [[unlikely]] {
        this->destroyBlock();
    }
}

template <typename T>
ASP_COLD void WeakPtr<T>::destroyBlock() {
    std::atomic_thread_fence(std::memory_order::acquire);

    ::operator delete(m_block);
}

// Shared from this impl

template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> sharedFromThis() {
        return m_weak.upgrade();
    }

    SharedPtr<const T> sharedFromThis() const {
        return m_weak.upgrade();
    }

    WeakPtr<T> weakFromThis() {
        return m_weak;
    }

    WeakPtr<const T> weakFromThis() const {
        return m_weak;
    }

protected:
    EnableSharedFromThis() = default;
    EnableSharedFromThis(const EnableSharedFromThis&) {}
    EnableSharedFromThis& operator=(const EnableSharedFromThis&) { return *this; }
    ~EnableSharedFromThis() = default;

private:
    mutable WeakPtr<T> m_weak{};

    template <class U>
    friend class SharedPtr;
};

template <typename T>
void SharedPtr<T>::_initSharedFromThis(T* ptr) {
    if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
        ptr->EnableSharedFromThis<T>::m_weak = *this;
    }
}

}
