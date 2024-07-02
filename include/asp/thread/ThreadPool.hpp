#pragma once

#include "Thread.hpp"
#include "../sync/Channel.hpp"
#include "../sync/Atomic.hpp"

namespace asp {

class ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool(size_t workers);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void pushTask(const Task& task);
    void pushTask(Task&& task);

    // Block the calling thread until all tasks have been completed.
    void join();

    // Returns `true` if the thread pool is currently doing any work, `false` if all threads are sleeping.
    bool isDoingWork();

    // Set the function that will be called when a thread throws an exception.
    void setExceptionFunction(const std::function<void(const std::exception&)>& f);

private:
    struct Worker {
        Thread<> thread;
        AtomicBool doingWork = false;
    };

    std::vector<Worker> workers;
    Channel<Task> taskQueue;
    std::function<void(const std::exception&)> onException;
};

}