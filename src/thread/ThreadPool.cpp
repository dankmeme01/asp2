#include <asp/thread/ThreadPool.hpp>
#include <asp/Log.hpp>

#ifdef ASP_IS_WIN
# include <Windows.h>
#endif

#ifdef ASP_IS_X86
# include <immintrin.h>
#else
# include <arm_acle.h>
#endif

static void microYield() {
#ifdef ASP_IS_X86
    _mm_pause();
#else
    __yield();
#endif
}

namespace asp {

ThreadPool::Worker::Worker(Thread<> thread) : thread(std::move(thread)) {}

ThreadPool::ThreadPool(size_t tc) : _storage(std::make_shared<Storage>()) {
    for (size_t i = 0; i < tc; i++) {
        Thread<> thread;
        thread.setLoopFunction([storage = _storage, i = i](auto&) {
            auto& worker = storage->workers.at(i);

            auto task = storage->taskQueue.popTimeout(time::Duration::fromMillis(10));

            if (!task) return;

            try {
                (*task)();
            } catch (const std::exception& e) {
                storage->onException(e);
            }

            storage->remainingWork.fetch_sub(1, std::memory_order::acq_rel);
        });

        _storage->workers.emplace_back(std::move(thread));
    }

    for (auto& worker : _storage->workers) {
        worker.thread.start();
    }
}

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}

ThreadPool::~ThreadPool() {
    // if taskQueue is null, this instance of ThreadPool was moved from.
    if (!_storage) return;

    m_destructing = true;

    try {
        this->join();

        // stop all threads and wait for them to terminate
        for (auto& worker : _storage->workers) {
            worker.thread.stop();
        }

        for (auto& worker : _storage->workers) {
            worker.thread.join();
        }

        _storage->workers.clear();
    } catch (const std::exception& e) {
        asp::log(LogLevel::Error, std::string("failed to cleanup thread pool: ") + e.what());
    }
}

void ThreadPool::pushTask(Task&& task) {
    this->_checkValid();

    _storage->remainingWork.fetch_add(1, std::memory_order::relaxed);
    _storage->taskQueue.push(std::move(task));
}

bool ThreadPool::allDead() {
    for (auto& worker : _storage->workers) {
#ifdef ASP_IS_WIN
        auto hnd = worker.thread.nativeHandle();
        DWORD code;
        if (GetExitCodeThread((HANDLE)hnd, &code) && code == STILL_ACTIVE) {
            return false;
        }
#else
        if (worker.thread.joinable()) return false;
#endif
    }

    return true;
}

void ThreadPool::join() {
    this->_checkValid();

    _storage->notifyWaiter.store(true, std::memory_order::release);

    auto isDone = [&] {
        // if we are destructing, it's possible that all threads are dead now, just terminate
        if (m_destructing && this->allDead()) {
            return true;
        }

        return _storage->remainingWork.load(std::memory_order::acquire) == 0;
    };

    while (!isDone()) {
        if (_storage->waiterSem.try_acquire_for(std::chrono::milliseconds{25})) {
            break;
        }
    }
}

void ThreadPool::joinSpin() {
    this->join();
}

bool ThreadPool::isDoingWork() {
    this->_checkValid();
    return _storage->remainingWork.load(std::memory_order::acquire) > 0;
}

void ThreadPool::setExceptionFunction(CopyableFunction<void(const std::exception&)> f) {
    this->_checkValid();

    for (auto& worker : _storage->workers) {
        worker.thread.setExceptionFunction(f);
    }

    _storage->onException = std::move(f);
}

void ThreadPool::_checkValid() {
    if (!_storage) throw std::runtime_error("Attempting to use an invalid ThreadPool that was moved from");
}

}