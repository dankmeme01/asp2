#pragma once

#include "Thread.hpp"
#include "../sync/Channel.hpp"
#include "../sync/Atomic.hpp"

namespace asp {

class ThreadPool {
public:
    using Task = std::function<void()>;

    // Initialize the thread pool with the given amount of threads.
    ThreadPool(size_t workers);
    // Initialize the thread pool with the amount of threads equal to the amount of CPUs on the machine.
    ThreadPool();

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = default;
    ThreadPool& operator=(ThreadPool&&) = default;

    void pushTask(const Task& task);
    void pushTask(Task&& task);

    // Block the calling thread until all tasks have been completed.
    void join();

    // Like `join`, but will spin instead of sleeping. Useful if the work is to be completed very quickly (milliseconds or less).
    void joinSpin();

    // Returns `true` if the thread pool is currently doing any work, `false` if all threads are sleeping.
    bool isDoingWork();

    // Set the function that will be called when a thread throws an exception.
    void setExceptionFunction(const std::function<void(const std::exception&)>& f);

private:
    struct Worker {
        Thread<> thread;
        AtomicBool doingWork = false;
    };

    struct Storage {
        std::vector<Worker> workers;
        Channel<Task> taskQueue;
        std::function<void(const std::exception&)> onException;
    };

    std::shared_ptr<Storage> _storage;
    bool m_destructing = false;

    void _checkValid();
    bool allDead();
};

}