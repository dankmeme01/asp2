#pragma once
#include <atomic>
#include <utility>

namespace asp {

template <typename T>
class WeakPtr;

template <typename T>
class PtrSwap;

using SharedPtrDtor = void(*)(void*);

template <typename T>
struct SharedPtrBlock {
    std::atomic<size_t> strong;
    std::atomic<size_t> weak;
    SharedPtrDtor dtor;
    T data;

    template <typename... Args>
    static SharedPtrBlock* create(Args&&... args) {
        auto mem = reinterpret_cast<SharedPtrBlock*>(::operator new(sizeof(SharedPtrBlock)));
        mem->strong.store(1);
        mem->weak.store(1);
        new (&mem->data) T(std::forward<Args>(args)...);
        mem->dtor = [](void* ptr) {
            reinterpret_cast<T*>(ptr)->~T();
        };
        return mem;
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

    size_t strongCount() const {
        return m_block ? m_block->strong.load(std::memory_order::relaxed) : 0;
    }

    size_t weakCount() const {
        return m_block ? m_block->weak.load(std::memory_order::relaxed) : 0;
    }

    T* get() const {
        return m_block ? &m_block->data : nullptr;
    }

    T& operator*() const {
        return m_block->data;
    }

    T* operator->() const {
        return &m_block->data;
    }

    operator bool() const {
        return m_block != nullptr;
    }

    bool operator==(std::nullptr_t) const {
        return m_block == nullptr;
    }

    bool operator==(const SharedPtr& other) const {
        return m_block == other.m_block;
    }

    bool operator==(const T& other) const {
        return m_block && (m_block->data == other);
    }

    void leak() {
        m_block = nullptr;
    }

    template <typename Y>
    operator SharedPtr<Y>() const requires std::is_convertible_v<T*, Y*> {
        return SharedPtr<Y>{reinterpret_cast<SharedPtrBlock<Y>*>(m_block)};
    }

private:
    friend class WeakPtr<T>;
    friend class PtrSwap<T>;
    SharedPtrBlock<T>* m_block;

    void release();
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
                return SharedPtr<T>::adoptFromRaw(m_block);
            }
        }

        return SharedPtr<T>();
    }

private:
    SharedPtrBlock<T>* m_block;

    void release();
};

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args&&... args) {
    auto block = SharedPtrBlock<T>::create(std::forward<Args>(args)...);
    return SharedPtr<T>::adoptFromRaw(block);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return make_shared<T>(std::forward<Args>(args)...);
}


template <typename T>
void SharedPtr<T>::release() {
    if (!m_block) return;

    if (m_block->strong.fetch_sub(1, std::memory_order::release) != 1) {
        return;
    }

    std::atomic_thread_fence(std::memory_order::acquire);

    auto weak = WeakPtr<T>::adoptFromRaw(m_block);
    m_block->dtor(&m_block->data);

    // dtor of weak will handle the actual deallocation, if necessary
}

template <typename T>
void WeakPtr<T>::release() {
    if (!m_block) return;

    if (m_block->weak.fetch_sub(1, std::memory_order::release) != 1) {
        return;
    }

    std::atomic_thread_fence(std::memory_order::acquire);

    ::operator delete(m_block);
}

}
